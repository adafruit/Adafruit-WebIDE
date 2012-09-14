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

exports.update = function (socket) {
  var self = this;
  self.get_version_info(function(err, version, update_url, update_notes) {
    if (err) {
      socket.emit('editor-update-complete', {editor_update_success: false, notes: update_notes});
    } else {
      execute_update(update_url, function(err, status) {
        setTimeout(function() {
          //allow for server restart time
          socket.emit('editor-update-complete', {editor_update_success: true, notes: update_notes});
        }, 5000);
      });
    }
  });
};

function download_archive(update_url, cb) {
  var download = request('https://dl.dropbox.com/s/wfg79l5f4old9pc/editor.tar.gz?dl=1');
  download.pipe(fs.createWriteStream(__dirname + '/../../editor.tar.gz'));
  download.on('error', function (e) {
    cb(e);
  });
  download.on('end', function () {
    cb();
  });
}

function execute_update(update_url, cb) {
  download_archive(update_url, function() {
    var editor_zip = __dirname + "/../../editor.tar.gz";
    console.log(editor_zip);
    extract_upate(editor_zip, function(err, status) {
      cb(err, status);
    });
  });
}

function extract_upate(file) {
  var child = exec('tar -zxvf ' + file + ' -C ' + __dirname + '/../../', function (err, stdout, stderr) {
      if (err || stderr) cb(err || stderr, false);
      cb(null, stdout);
  });
}

//this.check_for_updates();
//this.execute_program();