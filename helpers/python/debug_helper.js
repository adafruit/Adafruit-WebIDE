var spawn = require('child_process').spawn,
    exec = require('child_process').exec,
    net = require('net'),
    path = require('path'),
    debug_program, debug_client,
    client_connected = false,
    HOST = '127.0.0.1',
    PORT = 5000,
    buffer = '',
    enable_debug = false,
    VALID_COMMANDS = ['NEXT', 'STEP', 'RUN', 'QUIT', 'LOCALS', 'GLOBALS', 'ADD_BP', 'REMOVE_BP'];

exports.kill_debug = function kill_debug(should_enable, cb) {
  if (debug_program && debug_program.pid) {
    enable_debug = should_enable;
    var killer = exec('sudo kill ' + debug_program.pid, function(err, stdout, stderr) {
      console.log(err, stdout, stderr);
      console.log("Killed Debugger");
      cb();
    });
  }
  return;
};

exports.start_debug = function start_debug(file, socket) {
  var self = this;
  console.log("start_debug");
  console.log(!debug_program);
  if (!debug_program) {
    console.log('spawn debugger');
    debug_program = spawn("sudo", ["python", "debugger.py"], {cwd: __dirname});
    var buffer = "";
    debug_program.stdout.on('data', function(data) {
      console.log(data.toString());

      buffer += data.toString();

      if (buffer.indexOf("DEBUGGER READY") !== -1 && !client_connected) {
        console.log("DEBUGGER READY");
        connect_client(file, socket);
        console.log("after connect_client");
      }

    });

    debug_program.stderr.on('data', function(data) {
      console.log(data.toString());
      socket.emit('debug-error', {file: file, error: data});
    });

    debug_program.on('error', function(data) {
      console.log("DEBUG PROGRAM ERROR:");
      console.log(data);
    });

    debug_program.on('exit', function(code) {
      console.log('Debug Program Exit');
      console.log(code);
      debug_program = null;

      if (enable_debug) {
        self.start_debug(file, socket);
      }
    });
  } else {
    //console.log('resetting debugger');
    self.kill_debug(true, function() {
      //nothing to wait for here...exit will start a new process to debug
    });
  }
};

function get_file_path(file) {
  return path.resolve(__dirname + "/../../repositories/" + file.path.replace('/filesystem/', ''));
}

function connect_client(file, socket) {
  var file_path = get_file_path(file);
  console.log("connect_client");

  if (!debug_client) {
    debug_client = new net.Socket();
    debug_client.connect(PORT, HOST, function() {
      socket.emit('debug-client-connected');
      client_connected = true;
      console.log('connected to python debugger: ' + HOST + ':' + PORT);
      console.log(file_path);

      debug_client.write('DEBUG\t' + file_path + '\n');
    });

    debug_client.on('data', function(data) {
      buffer += data.toString();
      if (buffer.indexOf('\n')) {
        var temp_buff = buffer.split('\n');
        for (var i=0; i<temp_buff.length-1; i++) {
          console.log(JSON.parse(temp_buff[i]));
          socket.emit('debug-file-response', JSON.parse(temp_buff[i]));
        }

        buffer = temp_buff.slice(temp_buff.length);
      }
    });

    debug_client.on('error', function(data) {
      console.log('ERROR: ' + data);
    });

    // Add a 'close' event handler for the client socket
    debug_client.on('close', function() {
        console.log('Connection closed');
        client_connected = false;
        debug_client.destroy();
        debug_client = null;
        console.log('after connection close');
    });

    return;
  } else {
    debug_client.write('QUIT\n'); //Ensure the last debug session is cleared out
    debug_client.write('DEBUG\t' + file_path + '\n');
    console.log('already connected, initiated a new debug session');
    return;
  }
}

exports.client_disconnect = function client_disconnect() {
  if (debug_client && client_connected) {
    debug_client.destroy();
    debug_client = null;
  }
};

exports.debug_command = function(data, socket) {
  console.log(data.command);

  if (data.command === "QUIT") {
    this.kill_debug(false, function() {
      //nothing to wait for here...
    });
    return;
  }

  if (debug_client) {
    if (VALID_COMMANDS.indexOf(data.command) !== -1) {
      if (data.command === "ADD_BP" || data.command === "REMOVE_BP") {
        var file_path = get_file_path(data.file);
        data.command = data.command + '\t' + file_path + '~' + data.line_no;
      }

      debug_client.write(data.command + '\n');
    }
  } else {
    //TODO, handle re-connect
  }
};
