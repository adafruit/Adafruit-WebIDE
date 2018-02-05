var path = require('path'),
    fs = require('fs'),
    config = require('../config/config'),
    winston = require('winston'),
    exec = require('child_process').exec;

/*
 * Checks to see if an ssh key exists already.
 */
exports.has_ssh_key = function has_ssh_key(key_name, cb) {
  fs.exists(path.resolve(process.env['HOME'], '/.ssh/', key_name), function(exists) {
    if (exists) {
      cb(true);
    } else {
      cb(false);
    }
  });
};

/*
 * Generates an ssh key
 */
exports.generate_ssh_key = function(key_name, cb) {
  var self = this;
  self.has_ssh_key(key_name, function(exists) {
    if (exists) {
      cb();
    } else {
      exec("ssh-keygen -b 2048 -N '' -f ~/.ssh/" + key_name + "-t rsa -q", function(err, stdout, stderr) {
        //console.log(err, stdout, stderr);
        self.append_to_ssh_config(function() {
          cb();
        });
      });
    }
  });
};


exports.read_or_generate_key = function(key_name, cb) {
  var self = this;
  self.has_ssh_key(key_name, function(has_key) {
    if (has_key) {
      fs.readFile(process.env['HOME'] + '/.ssh/' + key_name, 'ascii', function(err,data){
        cb(data);
      });
    } else {
      self.generate_ssh_key(key_name, function() {
        fs.readFile(process.env['HOME'] + '/.ssh/' + key_name, 'ascii', function(err,data){
          cb(data);
        });
      });
    }
  });
};

exports.check_for_repository = function(repository, cb) {
  var repository_path = path.resolve(__dirname + '/../repositories/' + repository);
  fs.lstat(repository_path, function(err, stat) {
    if (stat && stat.isDirectory()) {
      cb(null, true);
    } else {
      cb(null, false);
    }
  });
};

exports.open_image = function(temp_path, cb) {
  var file_path = path.resolve(__dirname + '/../' + temp_path);

  fs.readFile(file_path, function(err, data){
    cb(err, data);
  });
};

/*
 * Copies recently uploaded file from tmp to the valid repositories folder
 */
exports.move_uploaded_file = function(temp_path, new_path, cb) {
  var is = fs.createReadStream(temp_path);
  var os = fs.createWriteStream(new_path);

  is.pipe(os);

  is.on("close", function() {
    fs.unlinkSync(temp_path);
    cb();
  });
};

/*
 * Simply renames a file or folder.
 */
exports.rename = function(old_path, new_path, cb) {
  fs.stat(new_path, function(err, stat) {
    if (stat) {
      cb("File already exists with that name");
    } else {
      fs.rename(old_path, new_path, cb);
    }
  });

};

/*
 * Copies the stock README into the my-pi-projects root folder.  This file is
 * opened when the editor is opened.
 */
exports.create_project_readme = function(cb) {
  var source = path.resolve(__dirname + '/../config/README.md');
  var destination = path.resolve(__dirname + '/../repositories/' + config.defaults.repository + '/' + config.defaults.readme);
  var file = {repository: config.defaults.repository, path: config.defaults.readme, name: config.defaults.readme};

  fs.lstat(destination, function(err, stat) {
    if (stat) cb("README already exists", file); //file exists

    var is = fs.createReadStream(source);
    var os = fs.createWriteStream(destination);

    is.pipe(os);

    is.on("close", function() {
      cb(null, file);
    });

  });
};

/*
 * Copies the stock .gitignore into the my-pi-projects root folder.
 */
exports.create_project_gitignore = function(cb) {
  var source = path.resolve(__dirname + '/../config/.gitignore');
  var destination = path.resolve(__dirname + '/../repositories/' + config.defaults.repository + '/' + config.defaults.gitignore);
  var file = {repository: config.defaults.repository, path: config.defaults.gitignore, name: config.defaults.gitignore};

  fs.lstat(destination, function(err, stat) {
    if (stat) cb(".gitignore already exists", file); //file exists

    var is = fs.createReadStream(source);
    var os = fs.createWriteStream(destination);
    is.pipe(os);

    is.on("close", function() {
      winston.debug("IN OS END");
      cb(null, file);
    });

  });
};
