var fs = require('fs');
var root
var dir = __dirname + "/../repositories/Adafruit-Raspberry-Pi-Python-Code";
var filter = ['.git'];

var walk = function(dir, done) {
  var results = [];
  fs.readdir(dir, function(err, list) {
    if (err) return done(err);
    var i = 0;
    (function next() {
      var file = list[i++];
      if (!file) return done(null, results);

      full_path = dir + '/' + file;
      relative_path = full_path.replace(__dirname + "/", '');
      //console.log(relative_path);
      fs.stat(full_path, function(err, stat) {
        if (stat && stat.isDirectory()) {
          if (filter.indexOf(file) === -1) {
            var dir = {data: file, children: []};
            walk(full_path, function(err, res) {
              dir.children = res;
              results.push(dir);
              next();
            });
          } else {
            next();
          }
        } else {
          results.push({data: file, attr: {"id": relative_path}});
          next();
        }
      });
    })();
  });
};

walk(dir, function(err, results) {
  if (err) throw err;
    console.log(results);
    console.log(results[0].children[3]);
});