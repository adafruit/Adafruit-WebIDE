var exec = require('child_process').exec,
  fs = require ('fs'),
  request = require('request'),
  config = require('../config/config');

exports.check_for_updates = function check_for_updates(socket) {
  var self = this;
  self.get_version_info(function(err, version, update_url, update_notes) {
    //console.log(err);
    var has_update = false;
    if (config.editor.version !== version) {
      console.log("Update available");
      has_update = true;
    } else {
      has_update = false;
    }

    socket.emit('editor-update-status', {has_update: has_update, url: update_url, notes: update_notes});
  });
};

exports.get_version_info = function(cb) {
  request(config.editor.version_url, function (err, response, body) {
    if (!err && response.statusCode == 200) {
      var version_info = body.split('\n');
      cb(null, version_info[0], version_info[1], version_info[2]);
    } else {
      cb(err);
    }
  });
};

exports.update = function (cb) {

};

function download_archive(update_url, cb) {
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

//this.check_for_updates();
//this.execute_program();