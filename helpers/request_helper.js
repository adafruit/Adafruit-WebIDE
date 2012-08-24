var request = require('request'),
    qs = require('querystring'),
    fs_helper = require('./fs_helper');

exports.post_ssh_key = function(profile, cb) {
  var url = "https://api.bitbucket.org/1.0/ssh-keys/";
  var oauth = { consumer_key: profile.consumer_key,
                consumer_secret: profile.consumer_secret,
                token: profile.token,
                token_secret: profile.token_secret };

  fs_helper.read_or_generate_key(function(data) {
    var params = { key: data.trim(), label: 'raspeditor'};

    request.post({
      url:url,
      body: qs.stringify(params),
      oauth:oauth
    }, function (e, r, body) {
      console.log(r.statusCode);
      console.log(body);
      cb (body);
    });
  });

};

exports.list_repositories = function(profile, cb) {
  var url = "https://api.bitbucket.org/1.0/user/repositories/";
  var oauth = { consumer_key: profile.consumer_key,
                consumer_secret: profile.consumer_secret,
                token: profile.token,
                token_secret: profile.token_secret };

  request.get({
    url:url,
    oauth:oauth,
    json: true
  }, function (e, r, body) {
    //console.log(e);
    //console.log(r.statusCode);
    //console.log(body);
    cb (e, body);
  });
};

exports.create_repository = function(profile, repository_name, cb) {
  var url = "https://api.bitbucket.org/1.0/repositories/";
  var oauth = { consumer_key: profile.consumer_key,
                consumer_secret: profile.consumer_secret,
                token: profile.token,
                token_secret: profile.token_secret };

  
  var params = { name: repository_name, scm: 'git'};

  request.post({
    url:url,
    body: qs.stringify(params),
    oauth:oauth
  }, function (e, r, body) {
    console.log(e);
    console.log(r.statusCode);
    console.log(body);
    cb (e, body);
  });
};