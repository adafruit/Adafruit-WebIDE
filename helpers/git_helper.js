var git = require('gitty');

exports.cloneAdafruitLibraries = function(cb) {
  git.clone("../repositories", 'git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git', function(output) {
    console.log(output);
    cb();
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