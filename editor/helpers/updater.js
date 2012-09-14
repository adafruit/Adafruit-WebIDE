var exec = require('child_process').exec,
  fs = require ('fs'),
  request = require('request');

function download_archive(cb) {
  var download = request('https://dl.dropbox.com/s/wfg79l5f4old9pc/editor.tar.gz?dl=1');
  download.pipe(fs.createWriteStream(__dirname + '/../../editor.tar.gz'));
  download.on('error', function (e) {
    cb(e);
  });
  download.on('end', function () {
      return cb();
  });
}

exports.execute_program = function execute_program() {
  download_archive(function() {
    var editor_zip = __dirname + "/../../editor.tar.gz";
    console.log(editor_zip);
    execute(editor_zip);
  });
};

function execute(file) {
  var child = exec('tar -zxvf ' + file + ' -C ' + __dirname + '/../../',
    function (error, stdout, stderr) {
      console.log('stdout: ' + stdout);
      console.log('stderr: ' + stderr);
      if (error !== null) {
        console.log('exec error: ' + error);
      }
  });
}

//this.execute_program();