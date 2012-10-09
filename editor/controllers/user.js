var redis = require("redis"),
    client = redis.createClient(),
    check = require('validator').check,
    sanitize = require('validator').sanitize;

exports.login = function(req, res){
  res.render('users/login', { title: 'test', user: req.user });
};

exports.logout = function(req, res){
  req.logout();
  res.redirect('/');
};

// Instructional page that displays the bitbucket setup steps,
// and inputs for OAuth and Git config
exports.setup = function(req, res) {
  client.hgetall('bitbucket_oauth', function (err, bitbucket) {
    client.hgetall('user', function (err, user) {
      var locals = {
        consumer_key: "",
        consumer_secret: "",
        name: "",
        email: "",
        hostname: ""
      };
      /*
      if (bitbucket) {
        locals.consumer_key = bitbucket.consumer_key;
        locals.consumer_secret = bitbucket.consumer_secret;
      }

      if (user) {
        locals.name = user.name;
        locals.email = user.email;
        locals.hostname = user.hostname;
      }*/
      
      res.render('users/setup', locals);
    });
  });
};

// Saves the bitbucket and git config setup information in Redis,
// submitted as a post from /setup
exports.submit_setup = function(req, res) {
  var key, secret, name, email, message;
  req.session.message = undefined;

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
    client.hmset("bitbucket_oauth", "consumer_key", key, "consumer_secret", secret, function() {
      client.hmset("user", "name", name, "email", email, function() {
        res.redirect('/login');
      });
    });
  } else {
    if (!req.session.message) {
      req.session.message = "Please set all fields, at the bottom of this page, in order to continue.";
    }
    res.redirect('/setup');
  }
};