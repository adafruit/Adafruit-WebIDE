var path = require('path'),
    db = require('../models/webideModel'),
    git = require('gitty'),
    Command = require('../node_modules/gitty/lib/command'),
    url = require('url'),
    winston = require('winston'),
    fs = require('fs'),
    fs_helper = require('./fs_helper'),
    ws_helper = require('./websocket_helper'),
    config = require('../config/config');

var REPOSITORY_PATH = path.resolve(__dirname + "/../repositories") + "/";
var push_queue = [], pushInterval, PUSH_TIMER = 30000;

/* extend gitty */
git.prototype.remove_recursive = function(path, callback) {
  var self = this;
  var cmd  = new Command(self, 'rm', ['-r', '--cached'], path);

  cmd.exec(function(error, stdout, stderr) {
    callback(error || stderr || null);
  });
};

git.prototype.move = function(source, destination, callback) {
  var self = this;
  var cmd  = new Command(self, 'mv', [], source, destination);

  cmd.exec(function(error, stdout, stderr) {
    callback(error || stderr || null);
  });
};

/*
 * Creates a simple queue that is used for pushing git changes to the remote repositories.
 * The queue is currently set using the PUSH_TIMER to delay the remote pushes.
 */
function push_queue_interval() {
  winston.debug('git push queue init');
  function push(repository_path, remote, branch, username) {
    var repo = git(repository_path);
    repo.push(remote, branch, function(err) {
      if (err) {
        winston.error(err);
        ws_helper.send_message(null, 'git-push-error', {err: "Error: Failure pushing code to remote repository"});
      }
      //winston.debug(obj);
    });
  }

  pushInterval = setInterval(function() {
    if (config.editor.offline || config.editor.github) {
      return;
    }

    while(push_queue.length > 0) {
      winston.debug('pushing code to remote repository');
      var element = push_queue.shift();
      push(element.repository_path, element.remote, element.branch, element.username);
    }
  }, PUSH_TIMER);
}
push_queue_interval();

/*
 * Clone the adafruit libraries defined in config/config.js.
 * We start the editor off with the Adafruit libraries that may be useful to beginners.
 */
exports.clone_adafruit_libraries = function(adafruit_repository, remote, cb) {
  fs_helper.check_for_repository(adafruit_repository, function(err, status) {
    if (!err && !status) {
      git.clone(REPOSITORY_PATH, remote, function(err) {
        winston.debug(err);
        cb(true);
      });
    } else {
      cb(false);
    }
  });
};

exports.clone_update_remote_push = function(repository_url, cb) {
  var self = this;

  var repository_name = path.basename(repository_url, '.git');

  self.clone_repository(repository_url, function(err, results) {
    winston.debug("clone repository locally: " + repository_name);
    cb(err, true);
  });
};

exports.clone_repository = function(repository_path, cb) {
  winston.debug("clone_repository", repository_path);
  var repository_url = url.parse(repository_path);

  winston.debug("cloning", repository_path);

  git.clone(REPOSITORY_PATH, repository_url.href, function(err, output) {
    cb(err, output);
  });
};

exports.create_local_repository = function(name, cb) {
  winston.debug("create_repository", name);
  //create directory
  var repository_path = REPOSITORY_PATH + name;
  if (!fs.existsSync(repository_path)){
    fs.mkdirSync(repository_path);
  }

  var repo = git(repository_path);
  repo.init(function(err, output) {
    cb(err, output);
  });
};

exports.validate_config = function validate_config(cb) {
  git.getConfig("user.email", function(err, email) {
    git.getConfig("user.name", function(err, name) {
      if (err) winston.error("git_helper.validate_config err", err);
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
      winston.debug('git config is valid');
      cb();
    } else {
      winston.error('git config is invalid');
      db.findOne({type: "user"}, function (err, user) {
        winston.debug("set_config user", user);
        git.setConfig("user.email", user.email, function(err, email) {
          git.setConfig("user.name", user.name, function(err, name) {
            winston.debug("git config set", email, name);
            cb();
          });
        });
      });
    }
  });

};

/*
 * Adds an additional remote to a repository.
 */
exports.add_remote = function(repository, remote_name, remote_url, cb) {
  var repo = git(REPOSITORY_PATH + repository);
  repo.addRemote(remote_name, remote_url, function(err, message) {
    winston.debug(err);
    cb(err, message);
  });
};

/*
 * git add a single file, or an array of files.
 * repository: the name of the repository that resides in the repositories folder.
 * files: the relative path of the files from the root of the repository.
 */
exports.add = function add(repository, files, cb) {
  if (!Array.isArray(files)) {
    files = [files];
  }
  var repository_path = REPOSITORY_PATH + repository;
  var repo = git(repository_path);
  repo.add(files, function(err, added) {
    winston.debug(err);
    cb(err, added);
  });
};

/*
 * git remove a single file, or an array of files.
 * repository: the name of the repository that resides in the repositories folder.
 * files: the relative path of the files from the root of the repository.
 */
exports.remove = function remove(repository, files, cb) {
  if (!Array.isArray(files)) {
    files = [files];
  }
  var repository_path = REPOSITORY_PATH + repository;
  var repo = git(repository_path);
  repo.remove(files, function(err, added) {
    winston.debug(err);
    cb(err, added);
  });
};

/*
 * git remove an entire directory, and it's contents.
 * repository: the name of the repository that resides in the repositories folder.
 * path: the relative path of the directory from the root of the repository.
 */
exports.remove_recursive = function remove_recursive(repository, path, cb) {
  var repository_path = REPOSITORY_PATH + repository;

  var repo = git(repository_path);
  repo.remove_recursive(path, function(err, data) {
    winston.debug(err);
    cb(err, data);
  });
};

/*
 * git move a single file or folder.
 * repository: the name of the repository that resides in the repositories folder.
 */
exports.move = function move(repository, source, destination, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  var repo = git(repository_path);
  repo.move(source, destination, function(err, message) {
    //winston.debug(obj);
    cb(err, message);
  });
};

/*
 * git commit the changes.
 * repository: the name of the repository that resides in the repositories folder.
 * message: The text to go along with the commit.
 */
exports.commit = function commit(repository, message, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  winston.debug(repository_path);
  var repo = git(repository_path);
  repo.commit(message, function(err, message) {
    //winston.debug(obj);
    cb(err, message);
  });
};

/*
 * git status a single file.
 * file: the relative path of the file from the root of the repository.
 */
exports.is_modified = function (file, cb) {
  var path_array = file.path.split('/');
  var repository = path_array[2];
  var repository_path = path.resolve(REPOSITORY_PATH, repository);
  var item_path = path_array.slice(3).join('/');

  var is_modified = false;
  var repo = git(repository_path);
  repo.status(function(err, output) {

    winston.debug(err, output);

    if (output.not_staged.length > 0) {
      output.not_staged.forEach(function(item, index) {
        if (item.file === item_path) {
          is_modified = true;
        }
      });
    }

    if (output.untracked.indexOf(item_path) !== -1) {
      is_modified = true;
    }

    cb(err, is_modified);
  });
};

/*
 * git status a single file to check if it's untracked.
 * file: the relative path of the file from the root of the repository.
 */
exports.is_untracked = function (file, cb) {
  var path_array = file.path.split('/');
  var repository = path_array[2];
  var repository_path = path.resolve(REPOSITORY_PATH, repository);
  var item_path = path_array.slice(3).join('/');

  winston.debug(item_path);

  var is_untracked = false;
  var repo = git(repository_path);
  repo.status(function(err, output) {

    winston.debug(err, output);

    if (output.untracked.indexOf(item_path) !== -1) {
      is_untracked = true;
    }

    cb(err, is_untracked);
  });
};

/*
 * git push the committed changes.  Adds it to the push queue.
 * repository: the name of the repository that resides in the repositories folder.
 */
exports.push = function push(repository, remote, branch, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  var key = repository + remote + branch;
  winston.debug('called push ' + key);

  //if the repository, remote and branch are already on the queue, just skip it...otherwise add it to the end
  if (push_queue.length > 0) {
    for (var i=0; i<push_queue.length; i++) {
      if (push_queue[i].key === key) {
        break;
      } else {
        winston.debug('added to queue ' + key);
        push_queue.push({
          key: key,
          repository_path: repository_path,
          repository: repository,
          remote: remote,
          branch: branch
        });
      }
    }
  } else {
    push_queue.push({
      key: key,
      repository_path: repository_path,
      repository: repository,
      remote: remote,
      branch: branch
    });
  }
  cb();
};

/*
 * git pull remote changes to the repository.
 * repository: the name of the repository that resides in the repositories folder.
 */
exports.pull = function pull(repository, remote, branch, cb) {
  var repository_path = REPOSITORY_PATH + repository;
  var repo = git(repository_path);
  repo.pull(remote, branch, function(err, message) {
    if (err) {
      winston.error(err);
      cb("Error: Failure updating from remote repository", message);
    } else {
      cb(null, message);
    }

  });
};

/*
 * Simply removes a file or directory, commits it, and pushes it out.
 */
exports.remove_commit_push = function(item, cb) {
  var self = this;
  winston.debug(item);
  var path_array = item.path.split('/');
  var repository = path_array[2];
  var item_path = path_array.slice(3).join('/');
  winston.debug(item_path);
  winston.debug(repository);


  if (item.type === 'directory') {
    self.remove_recursive(repository, item_path, function(err, status) {
      var commit_message = "Removed " + item.name;
      if (err && err.length > 0) {
        cb("Error: Failure removing folder from git", status);
        return;
      }
      self.commit(repository, commit_message,  function(err, status) {
        if (err && err.length > 0) {
          cb("Error: Failure comitting removed folder", status);
          return;
        }
        self.push(repository, "origin", "master", function() {
          cb();
        });
      });
    });
  } else {
    self.remove(repository, item_path, function(err, status) {
      if (err && err.length > 0) {
        cb("Error: Failure removing file from git", status);
        return;
      }
      var commit_message = "Removed " + item.name;
      self.commit(repository, commit_message,  function(err, status) {
        if (err && err.length > 0) {
          cb("Error: Failure comitting removed file", status);
          return;
        }
        self.push(repository, "origin", "master", function() {
          cb();
        });
      });
    });
  }
};

/*
 * Simply moves a file or directory, commits it, and pushes it out.
 */
exports.move_commit_push = function(item, cb) {
  var self = this;
  var path_array = item.path.split('/');
  var repository = path_array[2];
  var item_path = path_array.slice(3).join('/');
  var destination_path = item.destination.split('/').slice(3).join('/');

  self.is_untracked(item, function(err, is_untracked) {
    if (is_untracked) {
      item_path = path.resolve(REPOSITORY_PATH, repository, item_path);

      destination_path = path.resolve(REPOSITORY_PATH, repository, destination_path);
      fs_helper.rename(item_path, destination_path, function(err) {
        cb(err);
      });
    } else {
      self.move(repository, item_path, destination_path, function(err, status) {
        var commit_message = "Moved " + item.name;
        if (err) {
          winston.debug ("has error returning");
          cb("Error: Failure moving file (renaming)");
          return;
        }
        self.commit(repository, commit_message,  function(err, status) {
          winston.debug("Committed Moved File");
          if (err) {
            cb("Error: Failure comitting file into git");
            return;
          }
          self.push(repository, "origin", "master", function() {
            winston.debug("Pushed latest changes");
            cb();
          });
        });
      });
    }
  });

};

/*
 * Simply commits a file or directory, commits it, and pushes it out.
 */
exports.commit_push_and_save = function(file, commit_message, cb) {
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

  winston.debug(commit_message);



  self.add(repository, file_path, function(err, status) {
    winston.debug("added", err, status);
    if (err && err.length > 0) {
      cb("Error: Failure adding file to git", status);
      return;
    }
    self.commit(repository, commit_message,  function(err, status) {
      winston.debug("committed", err, status);
      if (err && err.length > 0) {
        cb("Error: Failure comitting file into git", status);
        return;
      }
      self.push(repository, "origin", "master", function() {
        winston.debug("added to push queue");
        cb();
      });
    });
  });
};
