var exec = require('child_process').exec,
  fs = require ('fs'),
  path = require('path'),
  request = require('request'),
  config = require('../config/config');

  fs.exists || (fs.exists = path.exists);

/*
 * Check for editor updates when connnecting to the editor.
 */
exports.check_for_updates = function check_for_updates(socket) {
  var self = this;

  console.log('check_for_updates');
  fs.exists(__dirname + '/../../update.lock', function(exists) {
    if (exists) {
      remove_lock_file(function (err) {
        socket.emit('editor-update-complete', {editor_update_success: true});
      });
    }
  });
          
  self.get_version_info(function(err, version, update_url, update_notes) {
    //console.log(err);
    var has_update = false;
    if (config.editor.version !== version) {
      console.log("Update available");
      has_update = true;
    } else {
      has_update = false;
    }
    if (update_notes) {
      request(update_notes, function (err, response, body) {
        socket.emit('editor-update-status', {has_update: has_update, version: version, url: update_url, notes: body});
      });
    }
  });
};

/*
 * Checks the remote version from the remote version.txt file defined in config/config.js
 */
exports.get_version_info = function(cb) {
  console.log('get_version_info');
  request(config.editor.version_url, function (err, response, body) {
    if (!err && response.statusCode == 200) {
      var version_info = body.split('\n');
      cb(null, version_info[0], version_info[1], version_info[2]);
    } else {
      cb(err);
    }
  });
};

/*
 * Start the update process, once the user clicks the update link.
 */
exports.update = function (socket) {
  var self = this;
  socket = socket;
  console.log('update');
  self.get_version_info(function(err, version, update_url, update_notes) {
    if (err) {
      remove_lock_file(function (err) {
        socket.emit('editor-update-complete', {editor_update_success: false, notes: update_notes});
      });
    } else {
      create_lock_file(function(err) {
        if (err) {
          remove_lock_file(function (err) {
            socket.emit('editor-update-complete', {editor_update_success: false, notes: update_notes});
          });
        } else {
          execute_update(update_url, socket, function(err, status) {
            //server restarting here...check for lock when re-connect
          });
        }
      });
    }
  });
};

/*
 * Creates an update.lock file to let the editor know it's being updated.
 */
function create_lock_file(cb) {
  fs.writeFile(__dirname + '/../../update.lock', '', function (err) {
    console.log('created lock file');
    if (err) cb(err);
    else cb(null);
  });
}

/*
 * Removes the update.lock file to let the editor know it's done being updated.
 */
function remove_lock_file(cb) {
  fs.unlink(__dirname + '/../../update.lock', function (err) {
    console.log('successfully deleted update.lock file');
    cb();
  });
}

/*
 * Downloads the tar.gz of the latest editor.  This file is defined in the remote version.txt file.
 */
function download_archive(update_url, socket, cb) {
  socket.emit('editor-update-download-start');
  console.log('download start');
  var download = request(update_url);
  download.pipe(fs.createWriteStream(__dirname + '/../../editor.tar.gz'));
  download.on('error', function (e) {
    cb(e);
  });
  download.on('end', function () {
    console.log('end download');
    socket.emit('editor-update-download-end');
    cb();
  });
}

/*
 * Handles the download, and unzipping of the latest editor code.
 */
function execute_update(update_url, socket, cb) {
  console.log('execute_update');
  download_archive(update_url, socket, function() {
    console.log('download response');
    var editor_zip = __dirname + "/../../editor.tar.gz";
    console.log(editor_zip);
    extract_upate(editor_zip, socket, function(err, status) {
      console.log('extract update response');
      cb(err, status);
    });
  });
}

/*
 * Extract the archive into the correct folder in a child process.  Overwrites existing files.
 * TODO: This should be improved to prevent corrupting a running installation on failure.
 *       Create the editor in a versioned folder, and point a symlink to that new version.
 */
function extract_upate(file, socket, cb) {
  var command = 'tar -zxvf ' + file + ' -C ' + __dirname + '/../../';
  socket.emit('editor-update-unpack-start');
  console.log('extract update');
  console.log(command);
  var child = exec('tar -zxvf ' + file + ' -C ' + __dirname + '/../../', function (err, stdout, stderr) {
    socket.emit('editor-update-unpack-end');
    console.log('err', err);
    console.log('stderr', stderr);
    console.log('stdout', stdout);
    if (err) cb(err, false);
    else cb(null, stdout);
  });
}

//this.check_for_updates();
//this.execute_program();