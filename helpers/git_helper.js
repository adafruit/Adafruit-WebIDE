var git = require('gitty'),
    url = require('url'),
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

exports.clone_repository = function(profile, repository_path, cb) {
  var repository_url = url.parse(repository_path);

  console.log("cloning", repository_path);
  git.clone(__dirname + "/../repositories", repository_url.href, function(output) {
    cb(output.error, output.message);
  });
};

exports.update_remote = function(profile, repository, cb) {
  var remote_url = "ssh://git@bitbucket.org/" + profile.username + "/" + repository.toLowerCase() + ".git";
  git.remote.update(__dirname + "/../repositories/" + repository, "origin", remote_url, function(output) {
    console.log(output);
    cb(output);
  });
};

exports.add = function add(repository, file, cb) {
  var repository_path = __dirname + "/../repositories/" + repository;
  git.add(repository_path, [file], function(obj) {
    console.log(obj);
    cb();
  });
};

exports.commit = function commit(repository, message, cb) {
  var repository_path = __dirname + "/../repositories/" + repository;
  git.commit(repository_path, message, function(obj) {
    console.log(obj);
    cb();
  });
};

exports.push = function push(repository, remote, branch, cb) {
  var repository_path = __dirname + "/../repositories/" + repository;
  git.push(repository_path, remote, branch, function(obj) {
    console.log(obj);
    cb();
  });
};

exports.commit_push_and_save = function(file, cb) {
  var that = this;
  var path_array = file.path.split('/');
  var repository = path_array[2];

  that.add(repository, file.name, function(err, status) {
    var commit_message = "Modified " + file.name;
    that.commit(repository, commit_message,  function(err, status) {
      that.push(repository, "origin", "master", function(err, status) {
        cb(status);
      });
    });
  });
};