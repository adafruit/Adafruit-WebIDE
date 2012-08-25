var spawn = require('child_process').spawn;

exports.execute_program = function(file, socket) {
  var file_path = __dirname + "/../repositories/" + file.path.replace('/filesystem/', '');
  console.log(file_path);
  if (file.extension === 'py') {
    execute_python(file_path, socket);
  } else if (file.extension === 'rb') {
    execute_ruby(file_path, socket);
  }
}

function execute_python(file, socket) {
  console.log('execute_python');
  var prog = spawn('python', [file]);

  prog.stdout.on('data', function(data) {
    socket.emit('program-output', {output: data.toString()});
  });

  prog.on('exit', function(code) {
    socket.emit('program-exit', {code: code});
  });
}

function execute_ruby(file, socket) {
  var prog = spawn('ruby', [file]);

  prog.stdout.on('data', function(data) {
    console.log(data.toString());
    socket.emit('program-output', {output: data.toString()});
  });

  prog.on('exit', function(code) {
    console.log(code);
    socket.emit('program-exit', {code: code});
  });
}

