var spawn = require('child_process').spawn,
    ws_helper = require('./websocket_helper'),
    //pty = require('pty.js'),
    path = require('path'),
    ipython, spawn_list = [];

/*exports.spawn_ipython = function() {
  ipython = pty.spawn('sudo', ['ipython']);
};*/

exports.execute_program = function(file, is_job) {

  console.log(file);
  if (file.extension === 'py') {
    execute_program(file, "python", is_job);
  } else if (file.extension === 'rb') {
    execute_program(file, "ruby", is_job);
  } else if (file.extension === 'js') {
    execute_program(file, "node", is_job);
  }
};

exports.stop_program = function(file, is_job) {
  var key = get_key(file);
  for (var i=0; i< spawn_list.length; i++) {
    if (spawn_list[i].key === key) {
      spawn_list.prog.kill();
      spawn_list.splice(i, 1);
    }
  }
};

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
  var generator_path = path.resolve(__dirname + "/python/encoder_combined.py");
  var args = ["python", generator_path, file_path];
  var program_output = "";
  var prog = spawn("sudo", args);

  prog.stdout.on('data', function(data) {
    program_output += data.toString();
    console.log(data.toString());
  });

  prog.stderr.on('data', function(data) {
    ws_helper.send_message(socket, 'trace-program-stderr', {output: data.toString()});
    console.log(data.toString());
  });

  prog.on('exit', function(code) {
    ws_helper.send_message(socket, 'trace-program-exit', {output: program_output});
  });
}

/*function execute_ipython(file, is_job) {
  var file_path = path.resolve(__dirname + "/../" + file.path.replace('\/filesystem\/', '\/repositories\/'));
  ipython.removeAllListeners('data');
  require('../server').get_socket(file.username, function(socket) {
    if (is_job) {
      ws_helper.send_message(socket, 'scheduler-start', {file: file});
    }
    ipython.on('data', function(data) {
      console.log(data);
      //data = data.replace(/\[0;.*?In\s\[.*?\[0m/, '~-prompt-~');
      //data = data.replace(/In\s\[.*?\]:/, '~-prompt-~');
      if (is_job) {
        ws_helper.send_message(socket, 'scheduler-executing', {file: file});
      } else {
        ws_helper.send_message(socket, 'program-stdout', {output: data});
      }
    });
  });
  ipython.write('run ');
  ipython.write(file_path);
  ipython.write('\r\n');

}*/

function get_key(file) {
  var key = "prog:" + file.path.replace(/\W/g, '');
  return key;
}

function get_cwd(file_path) {
  var split = file_path.split('/');
  split.splice(split.length-1, 1);
  return split.join('/');
}

function execute_program(file, type, is_job) {
  var file_path = path.resolve(__dirname + "/../" + file.path.replace('\/filesystem\/', '\/repositories\/'));

  console.log('execute_program');
  console.log(file_path);

  console.log(file);
  var cwd = get_cwd(file_path);
  var prog = spawn("sudo", [type, file_path], {cwd: cwd});
  var key = get_key(file);
  spawn_list.push({key: key, prog: prog});
  if (socket) {
    console.log('found socket, executing');
    handle_output(prog, file, is_job, socket);
  }
}

function handle_output(prog, file, is_job, socket) {
  if (is_job) {
    ws_helper.send_message(socket, 'scheduler-start', {file: file});
  }

  prog.stdout.on('data', function(data) {
    if (is_job) {
      console.log(data.toString());
      ws_helper.send_message(socket, 'scheduler-executing', {file: file});
    } else {
      console.log(data.toString());
      ws_helper.send_message(socket, 'program-stdout', {output: data.toString()});
    }
  });

  prog.stderr.on('data', function(data) {
    if (is_job) {
      console.log(data.toString());
      ws_helper.send_message(socket, 'scheduler-error', {file: file, error: data});
    } else {
      console.log(data.toString());
      ws_helper.send_message(socket, 'program-stderr', {output: data.toString()});
    }
  });

  prog.on('exit', function(code) {
    var key = get_key(file);
    for (var i=0; i< spawn_list.length; i++) {
      if (spawn_list[i].key === key) {
        spawn_list.splice(i, 1);
      }
    }

    if (is_job) {
      ws_helper.send_message(socket, 'scheduler-exit', {code: code, file: file});
    } else {
      ws_helper.send_message(socket, 'program-exit', {code: code});
    }

  });
}
