var git = require('gitty'),
    fs_helper = require('./fs_helper');

exports.clone_adafruit_libraries = function(cb) {
  fs_helper.check_for_repository('Adafruit-Raspberry-Pi-Python-Code', function(err, status) {
    if (!err && !status) {
    git.clone(__dirname + "/../repositories", 'git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git', function(output) {
      console.log(output);
      cb();
    });
    } else {
      cb();
    }
  });
};

exports.clone_repository = function(repository, cb) {
  var clone_path = "git@bitbucket.org:jwcooper/" + repository + ".git";
  console.log(clone_path);
  git.clone(__dirname + "/../repositories", clone_path, function(output) {
    console.log(output);
    cb();
  });
};