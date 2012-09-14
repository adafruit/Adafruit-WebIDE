var exec = require('child_process').exec;

exports.execute_program = function execute_program() {
  var editor_zip = __dirname + "/../../editor_v1.tar.bz2";
  console.log(editor_zip);
  execute(editor_zip);
};

function execute(file) {
  var child = exec('tar -jxvf ' + file + ' -C ' + __dirname + '/../../editor',
    function (error, stdout, stderr) {
      console.log('stdout: ' + stdout);
      console.log('stderr: ' + stderr);
      if (error !== null) {
        console.log('exec error: ' + error);
      }
  });
}

this.execute_program();