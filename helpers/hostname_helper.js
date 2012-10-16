var path = require('path'),
    fs = require('fs'),
    util = require('util'),
    config = require('../config/config');
    exec = require('child_process').exec;


    /*
 * Changes the hostname of the system
 */
exports.change_hostname = function(hostname, cb) {
  var self = this;
  var script_path = path.resolve(__dirname + "/../scripts/change-hostname.sh");
  var command = "sudo ./";
  exec("chmod +x " + script_path, function(err, stdout, stderr) {
    exec("sudo ./" + script_path + " " + hostname, function(err, stdout, stderr) {
      cb();
    });
  });
};