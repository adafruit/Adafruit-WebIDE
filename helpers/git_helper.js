var git = require('gitty'),
    fs_helper = require('./fs_helper');

exports.clone_adafruit_libraries = function(adafruit_repository, remote, cb) {
  fs_helper.check_for_repository(adafruit_repository, function(err, status) {
    if (!err && !status) {
    git.clone(__dirname + "/../repositories", remote, function(output) {
      console.log(output);
      cb();
    });
    } else {
      cb();
    }
  });
};

exports.clone_repository = function(profile, repository, cb) {
  var clone_path = "git@bitbucket.org:" + profile.username + "/" + repository + ".git";
  console.log(clone_path);
  git.clone(__dirname + "/../repositories", clone_path, function(output) {
    console.log(output);
    cb();
  });
};