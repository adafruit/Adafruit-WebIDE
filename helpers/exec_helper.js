var spawn = require('child_process').spawn,
    path = require('path');

exports.trace_program = function(file, socket) {
  var file_path = path.resolve(__dirname + "/../repositories/" + file.path.replace('/filesystem/', ''));
  console.log(file_path);  
  if (file.extension === 'py') {
    execute_python_trace(file_path, socket);
  } else if (file.extension === 'rb') {
    //execute_ruby(file_path, socket);
  } else if (file.extension === 'js') {
    //execute_javascript(file_path, socket);
  }
};

function execute_python_trace(file_path, socket) {
  console.log('execute_python_trace');
  var generator_path = path.resolve(__dirname + "/python/generate_json.py");
  var args = [generator_path, file_path];
  var program_output = "";
  var prog = spawn("python", args);

  prog.stdout.on('data', function(data) {
    program_output += data.toString();
    console.log(data.toString());
  });

  prog.stderr.on('data', function(data) {
    socket.emit('program-stderr', {output: data.toString()});
    console.log(data.toString());
  });

  prog.on('exit', function(code) {
    socket.emit('program-exit', {output: program_output});
  });

}