var redis = require("redis"),
    client = redis.createClient(),
    scripts_helper = require('../helpers/scripts_helper'),
    config = require('../config/config'),
    check = require('validator').check,
    sanitize = require('validator').sanitize;

exports.login = function(req, res){
  res.render('users/login', { title: 'test', user: req.user, github: config.editor.github });
};

exports.logout = function(req, res){
  req.logout();
  res.redirect('/');
};

// Instructional page that displays the bitbucket setup steps,
// and inputs for OAuth and Git config
exports.setup = function(req, res) {
  var locals = {
    consumer_key: "",
    consumer_secret: "",
    name: "",
    email: "",
    hostname: "",
    github: config.editor.github
  };

  res.render('users/setup', locals);
};

// Saves the bitbucket and git config setup information in Redis,
// submitted as a post from /setup
exports.submit_setup = function(req, res) {
  var key, secret, name, email, message;
  req.session.message = undefined;

  function common_setup(name, email) {
    client.hmset("user", "name", name, "email", email, function() {
      req.session.message = "Settings Successfully Configured.";
      res.redirect('/login');
    });
  }

  try {
    key = sanitize(req.body.key).xss().trim();
    secret = sanitize(req.body.secret).xss().trim();
    name = sanitize(req.body.name).xss().trim();
    email = sanitize(req.body.email).xss().trim();
    check(email).isEmail();
  } catch (e) {
    req.session.message = e.message;
    console.log(e.message);
  }

  if (key && secret && name && email) {
    if (config.editor.github) {
      client.hmset("github_oauth", "consumer_key", key, "consumer_secret", secret, function() {
        common_setup(name, email);
      });
    } else {
      client.hmset("bitbucket_oauth", "consumer_key", key, "consumer_secret", secret, function() {
        common_setup(name, email);
      });
    }
  } else {
    if (!req.session.message) {
      req.session.message = "Please set all fields, at the bottom of this page, in order to continue.";
    }
    res.redirect('/setup');
  }
};


exports.config = function(req, res) {
  client.hgetall('server', function (err, server) {
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
      client.hmset("server", "port", port, function() {
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