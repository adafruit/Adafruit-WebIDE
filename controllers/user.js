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

      console.log(locals);
      res.render('users/setup', locals);
    });
  });
};

exports.submit_setup = function(req, res) {
  console.log(req.body.key);
  console.log(req.body.secret);
  console.log(req.body.full_name);
  console.log(req.body.email);

  if (req.body.key && req.body.secret) {
    client.hmset("bitbucket_oauth", "consumer_key", req.body.key, "consumer_secret", req.body.secret, function() {
      client.hmset("user", "name", req.body.name, "email", req.body.email, function() {
        res.redirect('/login');
      });
    });
  }
};