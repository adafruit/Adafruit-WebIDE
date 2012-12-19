var spawn = require('child_process').spawn,
    net = require('net'),
    path = require('path'), 
    debug_program, debug_client,
    client_connected = false,
    HOST = '127.0.0.1',
    PORT = 5000,
    buffer = '',
    VALID_COMMANDS = ['NEXT', 'STEP', 'QUIT', 'LOCALS', 'GLOBALS'];

exports.kill_debug = function() {
  if (debug_program && debug_program.pid) {
    debug_program.kill('SIGHUP');
    debug_program = null;
  }
};

exports.start_debug = function(file, socket) {
  console.log("start_debug");
  console.log(!debug_program);
  if (!debug_program) {
    console.log('spawn debugger');
    debug_program = spawn("sudo", ["python", "debugger.py"], {cwd: __dirname});
    var buffer = "";
    debug_program.stdout.on('data', function(data) {
      console.log(data.toString());
      //socket.emit('program-stdout', {output: data.toString()});
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

    debug_program.on('exit', function(code) {
      console.log(code);
      socket.emit('debug-exit', {code: code});
    });
  } else {
    connect_client(file, socket);
  }
};

function connect_client(file, socket) {
  var file_path = path.resolve(__dirname + "/../../repositories/" + file.path.replace('/filesystem/', ''));
  console.log("connect_client");

  if (!debug_client) {
    debug_client = new net.Socket();
    debug_client.connect(PORT, HOST, function() {
      socket.emit('debug-client-connected');
      client_connected = true;
      console.log('connected to python debugger: ' + HOST + ':' + PORT);
      console.log(file_path);

      debug_client.write('DEBUG,' + file_path + '\n');
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
        debug_client = null;
    });

    return;
  } else {
    debug_client.write('QUIT\n'); //Ensure the last debug session is cleared out
    debug_client.write('DEBUG,' + file_path + '\n');
    console.log('already connected, initiated a new debug session');
    return;
  }
}

exports.client_disconnect = function() {
  console.log("111111111CLIENT DESTROYED CLIENT DESTROYED CLIENT DESTROYED");
  if (debug_client && client_connected) {
    debug_client.destroy();
    console.log("CLIENT DESTROYED CLIENT DESTROYED CLIENT DESTROYED");
    debug_client = null;    
  }
};

exports.debug_command = function(data, socket) {
  console.log(data.command);
  if (data.breakpoints) {
    console.log(data.breakpoints);
  }
  if (debug_client) {
    if (VALID_COMMANDS.indexOf(data.command) !== -1) {
      debug_client.write(data.command + '\n');
      //debug_client.write("LOCALS\n");
    }
  } else {
    //TODO, handle re-connect
  }
};
