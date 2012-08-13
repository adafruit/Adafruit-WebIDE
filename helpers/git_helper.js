var git = require('gitty');

exports.cloneAdafruitLibraries = function(cb) {
  git.clone("../repositories", 'git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git', function(output) {
    console.log(output);
    cb();
  });
};

exports.cloneRepository = function(profile, cb) {
  git.clone("../repositories", 'https://jwcooper@bitbucket.org/jwcooper/raspeditor.git', function(output) {
    console.log(output);
    cb();
  });
};