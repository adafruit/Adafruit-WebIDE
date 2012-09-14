var spawn = require('child_process').spawn;

exports.execute_program = function(file, socket) {
  var file_path = __dirname + "/../repositories/" + file.path.replace('/filesystem/', '');
  console.log(file_path);
  if (file.extension === 'py') {
    execute_python(file_path, socket);
  } else if (file.extension === 'rb') {
    execute_ruby(file_path, socket);
  } else if (file.extension === 'js') {
    execute_javascript(file_path, socket);
  }
};

function execute_python(file, socket) {
  console.log('execute_python');
  execute_program(file, socket, "python");
}

function execute_ruby(file, socket) {
  execute_program(file, socket, "ruby");
}

function execute_javascript(file, socket) {
  execute_program(file, socket, "node");
}

function execute_program(file, socket, type) {
  var prog = spawn(type, [file]);

  prog.stdout.on('data', function(data) {
    socket.emit('program-stdout', {output: data.toString()});
  });

  prog.stderr.on('data', function(data) {
    socket.emit('program-stderr', {output: data.toString()});
  });

  prog.on('exit', function(code) {
    socket.emit('program-exit', {code: code});
  });
}