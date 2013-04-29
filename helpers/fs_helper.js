var path = require('path'),
    fs = require('fs'),
    util = require('util'),
    config = require('../config/config');
    exec = require('child_process').exec;

/*
 * Checks to see if a bitbucket ssh key exists already.
 */
exports.has_ssh_key = function has_ssh_key(cb) {
  path.exists(process.env['HOME'] + '/.ssh/id_rsa_bitbucket.pub', function(exists) {
    if (exists) {
      cb(true);
    } else {
      cb(false);
    }
  });
};

/*
 * Generates an ssh key for Bitbucket
 */
exports.generate_ssh_key = function(cb) {
  var self = this;
  self.has_ssh_key(function(exists) {
    if (exists) {
      cb();
    } else {
      exec("ssh-keygen -b 2048 -N '' -f ~/.ssh/id_rsa_bitbucket -t rsa -q", function(err, stdout, stderr) {
        //console.log(err, stdout, stderr);
        self.append_to_ssh_config(function() {
          cb();
        });
      });
    }
  });
};

/*
 * Append bitbucket.org to the ssh config file to disable StrictHostKeyChecking.
 * This isn't great, but we need a way for beginners to get past the known_host checks.
 */
exports.append_to_ssh_config = function append_to_ssh_config(cb) {
  var ssh_config_file = process.env['HOME'] + '/.ssh/config';
  var identity_info = "Host bitbucket.org \r\n\tHostName bitbucket.org\r\n\tStrictHostKeyChecking no\r\n\tPreferredAuthentications publickey\r\n\tIdentityFile ~/.ssh/id_rsa_bitbucket";
  
  path.exists(ssh_config_file, function(exists) {
    if (exists) {
      //file exists, let's check if it has the bitbucket host in it, otherwise add it
      fs.readFile(ssh_config_file, 'ascii', function(err, data) {
        if (data.indexOf('bitbucket.org') !== -1) {
          cb();
        } else {
          var file = fs.createWriteStream(ssh_config_file, {'flags': 'a'});
          file.write(identity_info, function() {
            cb();
          });
        }
      });
    } else {
      fs.writeFile(ssh_config_file, identity_info, function(err) {
        if(err) console.log(err);
        cb();
      });
    }
  });
};

exports.read_or_generate_key = function(cb) {
  var self = this;
  self.has_ssh_key(function(has_key) {
    if (has_key) {
      fs.readFile(process.env['HOME'] + '/.ssh/id_rsa_bitbucket.pub', 'ascii', function(err,data){
        cb(data);
      });
    } else {
      self.generate_ssh_key(function() {
        fs.readFile(process.env['HOME'] + '/.ssh/id_rsa_bitbucket.pub', 'ascii', function(err,data){
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

  util.pump(is, os, function() {
      fs.unlinkSync(temp_path);
      cb();
  });
};

/*
 * Copies recently uploaded file from tmp to the valid repositories folder
 */
exports.rename_file = function(old_path, new_path, cb) {
  fs.rename(old_path, new_path, cb);
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
    util.pump(is, os, function(err) {
      console.log(err);
      cb(err, file);
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
    util.pump(is, os, function(err) {
      console.log(err);
      cb(err, file);
    });

  });
};