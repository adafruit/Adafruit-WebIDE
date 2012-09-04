var redis = require("redis"),
    client = redis.createClient();

exports.login = function(req, res){
  res.render('users/login', { title: 'test', user: req.user });
};

exports.logout = function(req, res){
  req.logout();
  res.redirect('/');
};

exports.setup = function(req, res) {
  res.render('users/setup');
};

exports.submit_setup = function(req, res) {
  console.log(req.body.key);
  console.log(req.body.secret);

  if (req.body.key && req.body.secret) {
    client.hmset("bitbucket_oauth", "consumer_key", req.body.key, "consumer_secret", req.body.secret, function() {
      res.redirect('/login');
    });
  }
};