var spawn = require('child_process').spawn,
    pty = require('pty.js'),
    path = require('path'),
    ipython;

exports.spawn_ipython = function() {
  ipython = pty.spawn('sudo', ['ipython']);
};

exports.execute_program = function(file, is_job) {
  
  console.log(file);
  if (file.extension === 'py') {
    execute_ipython(file, is_job);
  } else if (file.extension === 'rb') {
    execute_program(file, "ruby", is_job);
  } else if (file.extension === 'js') {
    execute_program(file, "node", is_job);
  }
};

function execute_ipython(file, is_job) {
  var file_path = path.resolve(__dirname + "/../" + file.path.replace('\/filesystem\/', '\/repositories\/'));
  ipython.removeAllListeners('data');
  require('../server').get_socket(file.username, function(socket) {
    if (is_job) {
      socket.emit('scheduler-start', {file: file});
    }
    ipython.on('data', function(data) {
      console.log(data);
      data = data.replace(/\[0;.*?In\s\[.*?\[0m/, '~-prompt-~');
      data = data.replace(/In\s\[.*?\]:/, '~-prompt-~');
      if (is_job) {
        socket.emit('scheduler-executing', {file: file});
      } else {
        socket.emit('program-stdout', {output: data});
      }
    });
  });
  ipython.write('run ');
  ipython.write(file_path);
  ipython.write('\r\n');

}

function execute_program(file, type, is_job) {
  var file_path = path.resolve(__dirname + "/../" + file.path.replace('\/filesystem\/', '\/repositories\/'));

  console.log('execute_program');
  console.log(file_path);

  require('../server').get_socket(file.username, function(socket) {
    var prog = spawn("sudo", [type, file_path]);
    if (socket) {
      console.log('found socket, executing');
      handle_output(prog, file, is_job, socket);
    }
  });
}

function handle_output(prog, file, is_job, socket) {
  if (is_job) {
    socket.emit('scheduler-start', {file: file});
  }

  prog.stdout.on('data', function(data) {
    if (is_job) {
      socket.emit('scheduler-executing', {file: file});
    } else {
      console.log(data.toString());
      socket.emit('program-stdout', {output: data.toString()});
    }
  });

  prog.stderr.on('data', function(data) {
    if (is_job) {
      socket.emit('scheduler-error', {file: file, error: data});
    } else {
      socket.emit('program-stderr', {output: data.toString()});
    }
  });

  prog.on('exit', function(code) {
    if (is_job) {
      socket.emit('scheduler-exit', {code: code, file: file});
    } else {
      socket.emit('program-exit', {code: code});
    }

  });    
}