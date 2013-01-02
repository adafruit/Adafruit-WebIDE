# The debugger.py contains methods from the Python built-in pdb debugger.
# It is also inspired, and contains code snippets from the GNU GPL v2 jpydbg remote debugging library plugin for jedit and netbeans.

import bdb
import sys
import os
import traceback
import socket
import types
import __builtin__
import json
import copy


# Server class heavily inspired by jpydbg, a GPL'd debugging framework for
# python that interfaces with java front-ends
# http://jpydbg.cvs.sourceforge.net/viewvc/jpydbg/jpydebugforge/src/python/jpydbg/dbgutils.py?view=markup
class Server:
    def __init__(self, connection):
        self._connection = connection
        self._buffer = ''

    def send_json(self, data):
        res = json.dumps(data, indent=None)
        try:
            self._connection.send(res + '\n')
        except socket.error:
            #not much we can do here...maybe TODO...
            pass
        except IOError:
            pass

    def read_net_buffer(self):
        """ reading on network socket """
        try:
            if (self._buffer.find('\n') != -1):
                return self._buffer ; # buffer still contains commands
            networkData = self._connection.recv(1024)
            if not networkData:  # capture network interuptions if any
                return None
            data = self._buffer + networkData
            return data
        except socket.error, (errno, strerror):
            print "recv interupted errno(%s) : %s" % (errno, strerror)
            return None

    def receive(self):
        """ receive a command back """
        data = self.read_net_buffer() ;
        # data reception from Ip
        while (data != None and data):
            eocPos = data.find('\n')
            nextPos = eocPos ;
            while (nextPos < len(data) and \
                    (data[nextPos] == '\n' or data[nextPos] == '\r')):  # ignore consecutive \n\r
                nextPos = nextPos + 1
            if (eocPos != -1):  # full command received in buffer
                self._buffer = data[nextPos:]  # cleanup received command from buffer
                returned = data[:eocPos]
                if (returned[-1] == '\r'):
                    return returned[:-1]
                return returned
            data = self.read_net_buffer() ;
        # returning None on Ip Exception
        return None

    def close(self):
        self._connection.close()


# Debugger is inspired by pdb and jpydbg.  Methods used, and modified.  Mostly a big fork of jpydbg
class Debugger(bdb.Bdb):
    def __init__(self, skip=None):
        bdb.Bdb.__init__(self, skip=skip)
        self._wait_for_mainpyfile = 0
        self._debug_active = 0
        self.exception_raised = 0
        self._connection = None
        self._socket = None
        self.cmd = None
        self.saved_stdout = None
        self.saved_stdin = None

    def reset(self):
        bdb.Bdb.reset(self)
        self.forget()

    #def close(self):
    #    if (self._connection):
    #        self._connection.close()

    def forget(self):
        self.lineno = None
        self.stack = []
        self.curindex = 0
        self.curframe = None

    def setup(self, f, t):
        self.forget()
        self.stack, self.curindex = self.get_stack(f, t)
        self.curframe = self.stack[self.curindex][0]
        # The f_locals dictionary is updated from the actual frame
        # locals whenever the .f_locals accessor is called, so we
        # cache it here to ensure that modifications are not overwritten.
        self.curframe_locals = self.curframe.f_locals

    def user_call(self, frame, argument_list):
        name = frame.f_code.co_name
        if not name:
            name = '???'
        fn = self.canonic(frame.f_code.co_filename)
        #print "user_call"
        if not fn:
            fn = '???'
        """This method is called when there is the remote possibility
        that we ever need to stop in this function."""
        if self._wait_for_mainpyfile:
            return
        if self.stop_here(frame):
            local_variables = self.get_variables("LOCALS", frame)
            global_variables = self.get_variables("GLOBALS", frame)
            #print self.stdout
            ret = dict(cmd=__builtin__.str(self.cmd), method="user_call", fn=fn, name=name,
                        line_no=__builtin__.str(frame.f_lineno), locals=local_variables, globals=global_variables)
            self._connection.send_json(ret)
            self.interaction(frame, None)

    def user_line(self, frame):
        #print "user_line"
        """This function is called when we stop or break at this line."""
        if self._wait_for_mainpyfile:
            if (self.mainpyfile != self.canonic(frame.f_code.co_filename)
                or frame.f_lineno <= 0):
                return
            self._wait_for_mainpyfile = 0
        #if self.bp_commands(frame):
        import linecache
        name = frame.f_code.co_name
        if not name:
            name = '???'
        fn = self.canonic(frame.f_code.co_filename)
        if not fn:
            fn = '???'
        # populate info to client side
        line = linecache.getline(fn, frame.f_lineno)
        local_variables = self.get_variables("LOCALS", frame)
        global_variables = self.get_variables("GLOBALS", frame)
        ret = dict(cmd=__builtin__.str(self.cmd), fn=fn, name=name,
                    line=line, line_no=__builtin__.str(frame.f_lineno), locals=local_variables, globals=global_variables)
        self._connection.send_json(ret)
        self.interaction(frame, None)

    def bp_commands(self, frame):
        """Call every command that was set for the current active breakpoint
        (if there is one).

        Returns True if the normal interaction function must be called,
        False otherwise."""
        # self.currentbp is set in bdb in Bdb.break_here if a breakpoint was hit
        if getattr(self, "currentbp", False) and \
               self.currentbp in self.commands:
            currentbp = self.currentbp
            self.currentbp = 0
            lastcmd_back = self.lastcmd
            self.setup(frame, None)
            for line in self.commands[currentbp]:
                self.onecmd(line)
            self.lastcmd = lastcmd_back
            #if not self.commands_silent[currentbp]:
                #self.print_stack_entry(self.stack[self.curindex])
            if self.commands_doprompt[currentbp]:
                self.cmdloop()
            self.forget()
            return
        return 1

    def user_return(self, frame, return_value):
        #print "user_return"
        """This function is called when a return trap is set here."""
        if self._wait_for_mainpyfile:
            return
        import linecache
        fn = self.canonic(frame.f_code.co_filename)
        if not fn:
            fn = '???'
        frame.f_locals['__return__'] = return_value
        line = linecache.getline(fn, frame.f_lineno)
        local_variables = self.get_variables("LOCALS", frame)
        global_variables = self.get_variables("GLOBALS", frame)
        ret = dict(cmd=__builtin__.str(self.cmd), fn=fn, return_value=__builtin__.str(return_value),
                    line=line, line_no=__builtin__.str(frame.f_lineno), locals=local_variables, globals=global_variables)
        self._connection.send_json(ret)
        self.interaction(frame, None)

    def user_exception(self, frame, exc_info):
        #print "user_exception"
        """This function is called if an exception occurs,
        but only if we are to stop at or just below this level."""
        if self._wait_for_mainpyfile:
            return
        if self.cmd == "NEXT" or self.cmd == "STEP":
            self.set_step()
            sys.settrace(self.trace_dispatch)
        else:
            self.populate_exception(exc_info)
            self.set_continue()
        #self.interaction(frame, None)

    def populate_exception(self, exc_info):
        # self.trace("exception populated")
        if (self.exceptionRaised == 0):  # exception not yet processed
            extype = exc_info[0]
            details = exc_info[1]

            # Deal With SystemExit in specific way to reflect debuggee's return
            if issubclass(extype, SystemExit):
                content = 'System Exit REQUESTED BY DEBUGGEE  =' + str(details)
            elif issubclass(extype, SyntaxError):
                content = __builtin__.str(details)
                error = details[0]
                compd = details[1]
                content = 'SOURCE:SYNTAXERROR:"' + \
                       __builtin__.str(compd[0]) + '":(' + \
                       __builtin__.str(compd[1]) + ',' + \
                       __builtin__.str(compd[2]) + \
                       ')' + ':' + error
            elif issubclass(extype, NameError):
                content = 'SOURCE:NAMEERROR:' + __builtin__.str(details)
            elif issubclass(extype, ImportError):
                content = 'SOURCE::IMPORTERROR:' + __builtin__.str(details)
            else:
                content = __builtin__.str(details)

            self.send_exception(content)
            self.exception_raised = 1  # set ExceptionFlag On

    def send_exception(self, exc_details):
        ret = dict(cmd="EXCEPTION", content=exc_details)
        self._connection.send_json(ret)

    def interaction(self, frame, traceback):
        self.setup(frame, traceback)
        self.set_until(self.curframe)
        #print "interaction"
        self.loop()
        self.forget()

    def loop(self):
        while (self.wait_for_input(self._connection.receive())):
            pass

    def wait_for_input(self, input):
        try:
            cmd, arg = input.split("\t")
        except:
            cmd = input
            arg = ''
        #print cmd
        #print arg

        if cmd:
            return self.handle_input(cmd, arg)
        else:
            #connection reset, let's setup a new listener
            self._socket.close()
            self.start_debugger()
        return 1

    def handle_input(self, cmd, arg):
        if cmd == "DEBUG":
            self.start_debug(arg)
            return 1
        elif cmd == "NEXT":
            return self.next()
        elif cmd == "STEP":
            return self.step()
        elif cmd == "RUN":
            return self.start_run()
        elif cmd == "ADD_BP":
            self.add_breakpoint(arg)
            return 1
        elif cmd == "REMOVE_BP":
            self.remove_breakpoint(arg)
            return 1
        elif cmd == "LOCALS":
            self.get_variables(cmd)
            return 1
        elif cmd == "GLOBALS":
            self.get_variables(cmd)
            return 1
        elif cmd == "QUIT":
            return self.quit()
        return 1

    def start_debug(self, filename):
        self.cmd = "DEBUG"
        self._debug_active = 1
        #self.saved_stdout = sys.stdout
        #self.saved_stdin = sys.stdin
        #sys.stdout = self
        #sys.stdin = self
        self._wait_for_mainpyfile = 1
        self.mainpyfile = self.canonic(filename)
        self._user_requested_quit = 0
        statement = 'execfile(%r)' % filename
        self.reset()
        #change running directory to location of file
        print filename
        print os.getcwd()
        debug_path = os.path.dirname(filename)
        sys.path.insert(0, debug_path)
        os.chdir(debug_path)
        print os.getcwd()
        try:
            self.run(statement)
            self._connection.send_json(dict(cmd="COMPLETE", content="EMPTY"))
            #sys.stdout = self.saved_stdout
            #sys.stdin = self.saved_stdin
            self._debug_active = 0
        except:
            tb, exctype, value = sys.exc_info()
            exc_trace = traceback.format_exception(tb, exctype, value)
            self.send_exception(exc_trace)
            self._connection.send_json(dict(cmd="ERROR_COMPLETE", content="EMPTY"))
            self._debug_active = 0
            pass

    def start_run(self):
        if self._debug_active:
            self.cmd = "RUN"
            self.set_continue()

    def next(self):
        if self._debug_active:
            self.cmd = "NEXT"
            self.set_next(self.curframe)
            return 0
        return 1

    def step(self):
        if self._debug_active:
            self.cmd = "STEP"
            self.set_step()
            return 0
        return 1

    def add_breakpoint(self, arg):
        filename, line_no = arg.split('~')
        canonic_file = self.canonic(filename)
        self.set_break(canonic_file, int(line_no))

    def remove_breakpoint(self, arg):
        filename, line_no = arg.split('~')
        canonic_file = self.canonic(filename)
        self.clear_break(canonic_file, int(line_no))

    def get_variables(self, cmd_type, frame):
        stack_list, size = self.get_stack(frame, None)
        if not stack_list:
            return []

        stack_list.reverse()
        stack_element = stack_list[size - 1]

        if cmd_type == "LOCALS":
            variables = copy.copy(stack_element[0].f_locals)
        elif cmd_type == "GLOBALS":
            variables = copy.copy(stack_element[0].f_globals)
        variable_list = []
        #print variables
        blocked_variables = set(['bdb', '__builtins__', 'socket', '__file__', '__builtin__', 'types', '__package__',
                                    'Server', 'sys', 'copy', 'Debugger', 'dbg', '__name__', 'traceback', 'json', 'os', '__doc__'])
        for key in variables.keys():
            if key in blocked_variables:
                variables.pop(key)

        for variable in variables.items():
            variable_list.append(dict(name=variable[0], content=__builtin__.str(variable[1]), type=self.get_var_type(variable[1])))
        #res = dict(cmd=cmd_type, variables=variable_list)
        #self._connection.send_json(res)
        return variable_list

    # acting as stdout => redirect to client side
    def write(self, toPrint):
        # transform eol pattern
        #if (toPrint == "\n"):
            #toPrint = "/EOL/"
        self._connection.send_json(dict(cmd="STDOUT", content=toPrint))

    # acting as stdout => redirect to client side
    def writeline(self, toPrint):
        # stdout redirection
        self.write(toPrint)
        self.write('\n')

      # stdout flush override
    def flush(self):
        pass

    def quit(self):
        if self._debug_active:
            self.cmd = "QUIT"
            sys.stdout = self.saved_stdout
            sys.stdin = self.saved_stdin
            self.reset()
            self.set_quit()
            self._debug_active = 0
            return 0
        else:
            return 1

    def start_debugger(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('127.0.0.1', 5000))
        s.listen(1)
        print "DEBUGGER READY"
        sys.stdout.flush()
        connection, addr = s.accept()
        self._socket = s
        self._connection = Server(connection)
        self.loop()

    # return true when selected element is composite candidate
    def is_composite(self, value):
        if isinstance(value, types.DictType):
            return 0
        elif isinstance(value, types.ListType):
            return 0
        elif isinstance(value, types.TupleType):
            return 0
        elif not (isinstance(value, types.StringType) or \
               isinstance(value, types.ComplexType) or \
               isinstance(value, types.FloatType) or \
               isinstance(value, types.IntType) or \
               isinstance(value, types.LongType) or \
               isinstance(value, types.NoneType) or \
               isinstance(value, types.UnicodeType)):
            return 1
        else:
            return 0

    # return true when selected element is composite candidate
    def get_simple_type(self, value):
        if isinstance(value, types.StringType):
            return 'String'
        elif isinstance(value, types.ComplexType):
            return 'ComplexNumber'
        elif isinstance(value, types.FloatType):
            return 'Float'
        elif isinstance(value, types.IntType):
            return 'Integer'
        elif isinstance(value, types.LongType):
            return 'Long'
        elif isinstance(value, types.NoneType):
            return 'None'
        elif isinstance(value, types.UnicodeType):
            return 'Unicode'
        else:
            return 'UNMANAGED DATA TYPE'

    # return true when selected element is map
    def is_map(self, value):
        if isinstance(value, types.DictType):
            return 1
        return 0

    # return true when selected element is List
    def is_list(self, value):
        if isinstance(value, types.ListType):
            return 1
        return 0

    # return true when selected element is List
    def is_tuple(self, value):
        if isinstance(value, types.TupleType):
            return 1
        return 0

    # return true when selected element is composite candidate
    def get_var_type(self, value):
        if self.is_composite(value):
            return 'COMPOSITE'
        else:
            if self.is_map(value):
                return 'MAP'
            elif self.is_list(value):
                return 'LIST'
            elif self.is_tuple(value):
                return 'TUPLE'
            return self.get_simple_type(value)

# When invoked as main program, invoke the debugger on a script
if __name__ == '__main__':
    dbg = Debugger()
    dbg.start_debugger()
