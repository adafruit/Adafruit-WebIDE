var fs_helper = require('../helpers/fs_helper'),
    git_helper = require('../helpers/git_helper');

exports.index = function(req, res){
  //var repository = req.params.repository;
  fs_helper.check_for_repository('/Adafruit-Raspberry-Pi-Python-Code', function(err, status) {
    if (!err && status) {
      res.render('editor/index');
    }else {
      git_helper.cloneAdafruitLibraries(function() {
        res.render('editor/index');
      });
    }
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