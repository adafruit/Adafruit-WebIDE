var path = require('path'),
    db = require('../models/webideModel'),
    scripts_helper = require('../helpers/scripts_helper'),
    config = require('../config/config'),
    check = require('validator').check,
    sanitize = require('validator').sanitize;

// Instructional page that displays the bitbucket setup steps,
// and inputs for OAuth and Git config
exports.setup = function(req, res) {
  var locals = {
    name: "",
    email: "",
    hostname: ""
  };

  res.render('users/setup', locals);
};

// Saves the bitbucket and git config setup information in Redis,
// submitted as a post from /setup
exports.submit_setup = function(req, res) {
  var key, secret, name, email, message;
  req.session.message = undefined;

  function common_setup(name, email) {
    db.update({"type": "user"}, {"type": "user", "name": name, "email": email}, { upsert: true }, function() {
      req.session.message = "Settings Successfully Configured.";
      res.redirect('/editor');
    });
  }

  try {
    name = sanitize(req.body.name).xss().trim();
    email = sanitize(req.body.email).xss().trim();
    check(email).isEmail();
  } catch (e) {
    req.session.message = e.message;
    console.log(e.message);
  }

  if (name && email) {
    common_setup(name, email);
  } else {
    if (!req.session.message) {
      req.session.message = "Please set all fields, at the bottom of this page, in order to continue.";
    }
    res.redirect('/setup');
  }
};


exports.config = function(req, res) {
  db.findOne({type: "server"}, function (err, server) {
      var locals = {
        hostname: "",
        wifi_ssid: "",
        wifi_password: "",
        port: (server ? (server.port || "") : "")
      };

      res.render('users/config', locals);
  });
};

// Saves the bitbucket and git config setup information in Redis,
// submitted as a post from /setup

//TODO: Refactor this...it's out of control!
exports.submit_config = function(req, res) {
  var key, secret, name, email, message;
  req.session.message = undefined;

  try {
    hostname = sanitize(req.body.hostname).xss().trim();
    wifi_ssid = sanitize(req.body.wifi_ssid).xss().trim();
    wifi_password = sanitize(req.body.wifi_password).xss().trim();
    port = sanitize(req.body.port).xss().trim();
    if (hostname) {
      check(hostname).len(3, 25);
    }
    if (port) {
      check(port).isNumeric().min(1).max(65535);
    }
  } catch (e) {
    req.session.message = e.message;
    console.log(e.message);
  }

  if (req.session.message) {
    res.redirect('/config');
  } else {
    //change the wifi without waiting for it
    if (wifi_ssid && wifi_password) {
      scripts_helper.change_wifi(wifi_ssid, wifi_password, function(err) {
        req.session.message = "Settings Successfully Configured.";
      });
    }
    if (port) {
      db.update({type: "server"}, { $set: {"port": port}}, {}, function() {

      });
    }

    if (hostname) {
      scripts_helper.change_hostname(hostname, function(err) {
        req.session.message = "Settings Successfully Configured.";
        res.redirect('http://' + hostname + '.local/login');
      });
    } else {
      if (port) {
        req.session.message = "Please restart the server for port changes to take effect.";
      }
      res.redirect('/login');
    }
  }
};

exports.set_datetime = function(req, res) {
  scripts_helper.set_datetime(function() {
    res.redirect('/login');
  });
};
