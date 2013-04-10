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
  var command = "sudo " + script_path + " \"" + hostname + "\"";
  console.log(command);
  exec("chmod +x " + script_path, function(err, stdout, stderr) {
    exec(command, function(err, stdout, stderr) {
      cb();
    });
  });
};

/*
 * Changes the WiFi of the system
 */
exports.change_wifi = function(ssid, password, cb) {
  var self = this;
  var script_path = path.resolve(__dirname + "/../scripts/change-wifi.sh");
  var command = "sudo " + script_path + " \"" + ssid + "\" \"" + password + "\"";
  console.log(command);
  exec("chmod +x " + script_path, function(err, stdout, stderr) {
    exec(command, function(err, stdout, stderr) {
      cb();
    });
  });
};

/*
 * Changes the WiFi of the system
 */
exports.set_datetime = function(cb) {
  var self = this;
  var script_path = path.resolve(__dirname + "/../scripts/set-datetime.sh");
  var command = "sudo " + script_path;
  console.log(command);
  exec("chmod +x " + script_path, function(err, stdout, stderr) {
    exec(command, function(err, stdout, stderr) {
      cb();
    });
  });
};