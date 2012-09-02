var fs_helper = require('../helpers/fs_helper'),
    git_helper = require('../helpers/git_helper');

exports.index = function(req, res) {
  res.render('editor/index');
};

exports.create_repository = function(req, res) {
  //TODO this is clearly, very, very insecure :)
  var repository_url = req.body.repository_url;

  git_helper.clone_update_remote_push(req.user, repository_url, function(err, status) {
    if (err) res.send(err, 404);
    else res.send(status, 200);
  });
};

exports.filesystem = function(req, res){
  var repository = req.query.repository;
  console.log(repository);
  //repository = "Adafruit-Raspberry-Pi-Python-Code";
  fs_helper.read_repository(repository, function(results) {
    res.send(results);
  });
};

exports.file = function(req, res) {
  //TODO this is clearly, very, very insecure :)
  var path = req.query.path;
  //console.log(path);
  fs_helper.open_file(path, function(results) {
    res.send(results);
  });
};