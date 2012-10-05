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
        email: ""
      };
      if (bitbucket) {
        locals.consumer_key = bitbucket.consumer_key;
        locals.consumer_secret = bitbucket.consumer_secret;
      }

      if (user) {
        locals.name = user.name;
        locals.email = user.email;
      }
      
      res.render('users/setup', locals);
    });
  });
};

// Saves the bitbucket and git config setup information in Redis,
// submitted as a post from /setup
exports.submit_setup = function(req, res) {
  var key = sanitize(req.body.key).xss().trim();
  var secret = sanitize(req.body.secret).xss().trim();
  var name = sanitize(req.body.name).xss().trim();
  var email = sanitize(req.body.email).xss().trim();

  if (key && secret) {
    client.hmset("bitbucket_oauth", "consumer_key", key, "consumer_secret", secret, function() {
      client.hmset("user", "name", name, "email", email, function() {
        res.redirect('/login');
      });
    });
  }
};