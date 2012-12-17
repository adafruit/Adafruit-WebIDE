var spawn = require('child_process').spawn,
    net = require('net'),
    path = require('path'), 
    debug_program, debug_client,
    client_connected = false,
    HOST = '127.0.0.1',
    PORT = 5000,
    VALID_COMMANDS = ['NEXT', 'QUIT', 'LOCALS', 'GLOBALS'];

exports.kill_debug = function() {
  if (debug_program && debug_program.pid) {
    debug_program.kill('SIGHUP');
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

      if (buffer.indexOf("DEBUGGER READY") !== -1) {
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

      /*client.write('NEXT\n');
      client.write('LOCALS\n');
      client.write('NEXT\n');
      client.write('LOCALS\n');
      client.write('NEXT\n');
      client.write('LOCALS\n');
      client.write('NEXT\n');
      client.write('GLOBALS\n'); 
      client.write('NEXT\n');
      client.write('NEXT\n');
      client.write('NEXT\n');      
      client.write('QUIT\n');*/
      //client.write('DEBUG,/Users/jwcooper/dev/apps/OccEditor/helpers/python/temp/test.py\n');
      //client.write('NEXT\n');    
    });

    // Add a 'data' event handler for the client socket
    // data is what the server sent to this socket
    debug_client.on('data', function(data) {
        
      console.log('DATA: ' + data);
      socket.emit('debug-file-response', JSON.parse(data.toString()));
        // Close the client socket completely
        //client.destroy();
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
    console.log('already connected, do something');
    return;
  }
}

exports.debug_command = function(command, socket) {
  console.log(command);
  if (debug_client) {
    if (VALID_COMMANDS.indexOf(command) !== -1) {
      debug_client.write('NEXT\n');
    }
  } else {
    //TODO, handle re-connect
  }
};
