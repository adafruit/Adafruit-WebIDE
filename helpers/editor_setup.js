var exec = require('child_process').exec,
  fs = require ('fs'),
  path = require('path'),
  git_helper = require('./git_helper'),
  fs_helper = require('./fs_helper'),
  redis = require('redis'),
  client = redis.createClient(),
  request_helper = require('./request_helper'),
  config = require('../config/config');

  fs.exists || (fs.exists = path.exists);

  exports.setup_github = function(socket) {
    git_helper.set_config(function() {
      this.setup_adafruit_libraries(socket);
    });
  };

  exports.setup_adafruit_libraries = function(socket) {
    git_helper.clone_adafruit_libraries(config.adafruit.repository, config.adafruit.remote, function(cloned_libraries) {
      socket.emit("self-check-message", "Cloning remote Adafruit repository");
      //cloned_libraries is false if they already existed...if false, let's pull the latest version of the adafruit libraries
      if (!cloned_libraries) {
        git_helper.pull(config.adafruit.repository, "origin", "master", function() {
          socket.emit("self-check-message", "Adafruit repository updated");
        });
      }
    });
  };

  exports.offline_health_check = function(socket) {
    client.hgetall('editor:settings', function(err, settings) {

      if (settings) {
        settings.offline = true;
      } else {
        settings = {offline: true};
      }
      console.log("getting settings", settings);
      socket.emit("self-check-settings", settings);
    });

    console.log('self-check-complete');
    socket.emit('self-check-complete');
  };

  //TODO this is a terrible mess..clean this up, no reason to have these big blocks of callbacks...uffda.
  exports.health_check = function(socket, profile) {
    var that = this;
    console.log(config.editor.offline);

    if (config.editor.github) {
      this.setup_github(socket);
    }

    if (config.editor.offline || config.editor.github) {
      this.offline_health_check(socket);
      return;
    }

    var project_repository = 'git@bitbucket.org:' + profile.username + '/my-pi-projects.git';
    console.log(project_repository);

    client.hgetall('editor:settings', function(err, settings) {
      if (typeof settings === 'undefined' || !settings) {
        settings = {};
      }

      console.log("getting settings", settings);
      socket.emit("self-check-settings", settings);
    });

    //check if the adafruit libraries exist, if not, clone them.
    request_helper.post_ssh_key(profile, function(err, response) {
      that.setup_adafruit_libraries(socket);

    git_helper.set_config(function() {
      request_helper.list_repositories(profile, function(err, list) {
        var exists = list.some(function(repository) {
          return (repository.name.toLowerCase() === config.defaults.repository.toLowerCase());
        });

        if (!exists) {
          request_helper.create_repository(profile, config.defaults.repository, function(err, response) {
            socket.emit("self-check-message", "Created my-pi-projects in Bitbucket");

            git_helper.clone_repository(project_repository, function(err, response) {
              socket.emit("self-check-message", "Cloned my-pi-projects on local system");
              fs_helper.create_project_gitignore(function(err, file) {
                socket.emit("self-check-message", "Added .gitignore in my-pi-projects");
                fs_helper.create_project_readme(function(err, file) {
                  socket.emit("self-check-message", "Added README.md in my-pi-projects");
                  if (err) console.log(err);
                  console.log(file);

                  git_helper.commit_push_and_save(file, "Added README.md and .gitignore", null, function(err, response) {
                    socket.emit("self-check-message", "Pushed changes to my-pi-projects to Bitbucket");
                    socket.emit('self-check-complete');
                  });
                });
              });
            });
          });
        } else {
          //check if repository exists here
          var my_repository = "my-pi-projects";
          fs_helper.check_for_repository(my_repository, function(err, exists) {
            socket.emit("self-check-message", "Validated my-pi-projects");
            if (exists) {
              git_helper.pull(my_repository, "origin", "master", function(err, status) {
                socket.emit("self-check-message", "Updated my-pi-projects");
                socket.emit('self-check-complete');
              });
            } else {
              git_helper.clone_repository(project_repository, function(err, response) {
                socket.emit("self-check-message", "Cloned my-pi-projects on local system");
                fs_helper.create_project_gitignore(function(err, file) {
                  socket.emit("self-check-message", "Added .gitignore in my-pi-projects");
                  fs_helper.create_project_readme(function(err, file) {
                    socket.emit("self-check-message", "Added README.md in my-pi-projects");
                    console.log(file);
                    if (err) console.log(err);

                    git_helper.commit_push_and_save(file, "Added README.md and .gitignore", null, function(err, response) {
                      socket.emit("self-check-message", "Pushed changes to my-pi-projects to Bitbucket");
                      socket.emit('self-check-complete');
                    });
                  });
                });
              });
            }
          });

        }

      }); // end of list repositories
    }); //end of git set config
  }); //end of post ssh key
    
  };