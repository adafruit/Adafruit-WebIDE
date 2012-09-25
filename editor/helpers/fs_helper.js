var path = require('path'),
    fs = require('fs'),
    util = require('util'),
    config = require('../config/config');
    exec = require('child_process').exec;

exports.has_ssh_key = function has_ssh_key(cb) {
  path.exists(process.env['HOME'] + '/.ssh/id_rsa_bitbucket.pub', function(exists) {
    if (exists) {
      cb(true);
    } else {
      cb(false);
    }
  });
};

exports.generate_ssh_key = function(cb) {
  var self = this;
  self.has_ssh_key(function(exists) {
    if (exists) {
      cb();
    } else {
      //TODO generate key
      exec("ssh-keygen -b 2048 -N '' -f ~/.ssh/id_rsa_bitbucket -t rsa -q", function(err, stdout, stderr) {
        //console.log(err, stdout, stderr);
        self.append_to_ssh_config(function() {
          cb();
        });
      });
    }
  });
};

exports.append_to_ssh_config = function append_to_ssh_config(cb) {
  var ssh_config_file = process.env['HOME'] + '/.ssh/config';
  var identity_info = "Host bitbucket.org \r\n\tHostName bitbucket.org\r\n\tStrictHostKeyChecking no\r\n\tPreferredAuthentications publickey\r\n\tIdentityFile ~/.ssh/id_rsa_bitbucket";
  path.exists(ssh_config_file, function(exists) {
    if (exists) {
      var file = fs.createWriteStream(ssh_config_file, {'flags': 'a'});
      file.write(identity_info, function() {
        cb();
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
  var repository_path = path.resolve(__dirname + '/../../repositories/' + repository);
  fs.lstat(repository_path, function(err, stat) {
    if (stat && stat.isDirectory()) {
      cb(null, true);
    } else {
      cb(null, false);
    }
  });
};

function build_file_structure(path, cb) {
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

  walk(path, function(err, results) {
    if (err) throw err;
      cb(results);
  });
}

exports.read_repository = function(repository, cb) {
  
  var repository_path = path.resolve(__dirname + "/../../repositories/" + repository);
  build_file_structure(repository_path, function(results) {
    cb(results);
  });
};

exports.open_file = function(path, cb) {
  fs.readFile(__dirname + '/' + path, 'ascii', function(err,data){
    cb(data);
  });
};

exports.move_uploaded_file = function(temp_path, new_path, cb) {
  fs.rename(temp_path, new_path, function(err) {
    cb(err);
  });
};

exports.create_project_readme = function(cb) {
  var source = path.resolve(__dirname + '/../config/README.md');
  var destination = path.resolve(__dirname + '/../../repositories/' + config.defaults.repository + '/' + config.defaults.readme);
  console.log(source);
  console.log(destination);
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