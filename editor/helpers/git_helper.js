var git = require('gitty'),
    url = require('url'),
    path = require('path'),
    fs_helper = require('./fs_helper'),
    redis = require("redis"),
    client = redis.createClient(),
    request_helper = require('./request_helper');

var REPOSITORY_PATH = path.resolve(__dirname + "/../../repositories") + "/";

exports.clone_adafruit_libraries = function(adafruit_repository, remote, cb) {
  fs_helper.check_for_repository(adafruit_repository, function(err, status) {
    if (!err && !status) {
      git.clone(REPOSITORY_PATH, remote, function(output) {
        console.log(output);
        cb(true);
      });
    } else {
      cb(false);
    }
  });
};

exports.clone_update_remote_push = function(profile, repository_url, cb) {
  var self = this;
  //console.log(profile);
  var repository_name = path.basename(repository_url, '.git');
  console.log(repository_name);
  console.log(repository_url);

  request_helper.list_repositories(profile, function(err, list) {
    var exists = list.some(function(repository) {
      return (repository.name === repository_name);
    });
    if (!exists) {
      //TODO need better error handling eventually
      request_helper.create_repository(profile, repository_name, function(err, response) {
        console.log("created repository in bitbucket: " + repository_name);
        self.clone_repository(profile, repository_url, function(err, results) {
          console.log("clone repository locally: " + repository_name);
          self.update_remote(profile, repository_name, function(err, response) {
            console.log("updated remote for repository: " + repository_name);
            self.push(repository_name, "origin", "master", function(err, response) {
              console.log("git push for repository: " + repository_name);
              cb(null, true);
            });
          });
        });
      });
    } else {
      cb("Repository Already Exists in Bitbucket.", false);
    }
  });
};

exports.clone_repository = function(profile, repository_path, cb) {
  console.log(repository_path);
  var repository_url = url.parse(repository_path);

  console.log("cloning", repository_path);
  request_helper.post_ssh_key(profile, function(err, response) {
    console.log(err, response);
    git.clone(REPOSITORY_PATH, repository_url.href, function(output) {
      cb(output.error, output.message);
    });
  });

};

exports.validate_config = function validate_config(cb) {
  git.config("user.email", null, function(err, email) {
    git.config("user.name", null, function(err, name) {
      if (err) console.log("validate_config err", err);

      if (name && email) {
        cb(true);
      } else {
        cb(false);
      }
    });
  });
};

exports.set_config = function(cb) {
  var self = this;
  self.validate_config(function(is_valid) {
    if (is_valid) {
      console.log('git config is valid');
      cb();
    } else {
      console.log('git config is invalid');
      client.hgetall('user', function (err, user) {
        console.log("set_config user", user);
        git.config("user.email", user.email, function(err, email) {
          git.config("user.name", user.name, function(err, name) {
            console.log("git config set", email, name);
            cb();
          });
        });
      });
    }
  });

};

exports.update_remote = function(profile, repository, cb) {
  var remote_url = "ssh://git@bitbucket.org/" + profile.username + "/" + repository.toLowerCase() + ".git";
  git.remote.update(REPOSITORY_PATH + repository, "origin", remote_url, function(output) {
    //console.log(output);
    cb(output.error, output.message);
  });
};

exports.add_remote = function(repository, remote_name, remote_url, cb) {
  git.remote.add(REPOSITORY_PATH + repository, remote_name, remote_url, function(output) {
    //console.log(output);
    cb(output.error, output.message);
  });
};

exports.add = function add(repository, files, cb) {
  if (!Array.isArray(files)) {
    files = [files];
  }
  var repository_path = REPOSITORY_PATH + repository;
  git.add(repository_path, files, function(output) {
    //console.log(output);
    cb(output.errors, output.added);
  });
};

exports.remove = function remove(repository, files, cb) {
  if (!Array.isArray(files)) {
    files = [files];
  }
  var repository_path = REPOSITORY_PATH + repository;
  git.remove(repository_path, files, function(output) {
    console.log(output);
    cb(output.errors, output.added);
  });
};

exports.remove_recursive = function remove_recursive(repository, path, cb) {
  var repository_path = REPOSITORY_PATH + repository;

  git.remove_recursive(repository_path, path, function(output) {
    console.log(output);
    cb(output.errors, output.added);
  });
};

exports.move = function move(repository, source, destination, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  git.move(repository_path, source, destination, function(obj) {
    //console.log(obj);
    cb(obj.error, obj.message);
  });
};

exports.commit = function commit(repository, message, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  console.log(repository_path);
  git.commit(repository_path, message, function(obj) {
    //console.log(obj);
    cb(obj.error, obj.message);
  });
};

exports.push = function push(repository, remote, branch, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  git.push(repository_path, remote, branch, function(obj) {
    //console.log(obj);
    cb(obj.error, obj.message);
  });
};

exports.pull = function pull(repository, remote, branch, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  git.pull(repository_path, remote, branch, function(obj) {
    //console.log(obj);
    cb(obj.error, obj.message);
  });
};


exports.remove_commit_push = function(item, cb) {
  var self = this;
  console.log(item);
  var path_array = item.path.split('/');
  var repository = path_array[2];
  var item_path = path_array.slice(3).join('/');
  console.log(item_path);
  console.log(repository);


  if (item.type === 'directory') {
    self.remove_recursive(repository, item_path, function(err, status) {
      var commit_message = "Removed " + item.name;
      self.commit(repository, commit_message,  function(err, status) {
        self.push(repository, "origin", "master", function(err, status) {
          cb(err, status);
        });
      });
    });
  } else {
    self.remove(repository, item_path, function(err, status) {
      var commit_message = "Removed " + item.name;
      self.commit(repository, commit_message,  function(err, status) {
        self.push(repository, "origin", "master", function(err, status) {
          cb(err, status);
        });
      });
    });
  }
};

exports.move_commit_push = function(item, cb) {
  var self = this;
  var path_array = item.path.split('/');
  var repository = path_array[2];
  var item_path = path_array.slice(3).join('/');
  var destination_path = item.destination.split('/').slice(3).join('/');

  self.move(repository, item_path, destination_path, function(err, status) {
    var commit_message = "Moved " + item.name;
    console.log(commit_message);
    self.commit(repository, commit_message,  function(err, status) {
      console.log("Committed Moved File");
      self.push(repository, "origin", "master", function(err, status) {
        console.log("Pushed latest changes");
        cb(err, status);
      });
    });
  });
};

exports.commit_push_and_save = function(file, cb) {
  var self = this,
      path_array, repository, file_path;
  if (!file.repository) {
    path_array = file.path.split('/');
    repository = path_array[2];
    file_path = path_array.slice(3).join('/');
  } else {
    repository = file.repository;
    file_path = file.path;
  }

  self.add(repository, file_path, function(err, status) {
    console.log("added", err, status);
    var commit_message = "Modified " + file.name;
    self.commit(repository, commit_message,  function(err, status) {
      console.log("committed", err, status);
      self.push(repository, "origin", "master", function(err, status) {
        console.log("pushed", err, status);
        cb(status);
      });
    });
  });
};