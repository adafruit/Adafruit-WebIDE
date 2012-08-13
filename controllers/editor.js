var fs_helper = require('../helpers/fs_helper');

exports.index = function(req, res){
  var repository = req.params.repository;

  //if (req.user) {
    res.render('editor/index', { repository: repository });
  //}
};

exports.filesystem = function(req, res){
  var repository = req.params.repository;
  console.log(repository);
  repository = "Adafruit-Raspberry-Pi-Python-Code";
  fs_helper.read_repository(repository, function(results) {
    res.send(results);
  });
  
};