var path = require('path'),
    winston = require('winston'),
    db = require('../models/webideModel'),
    exec = require('child_process').exec,
    fs = require ('fs'),
    git_helper = require('./git_helper.js'),
    fs_helper = require('./fs_helper'),
    ws_helper = require('./websocket_helper'),
    config = require('../config/config');

  fs.exists || (fs.exists = path.exists);

  exports.setup_adafruit_libraries = function(ws) {
    git_helper.clone_adafruit_libraries(config.adafruit.repository, config.adafruit.remote, function(cloned_libraries) {
      ws_helper.send_message(ws, "self-check-message", "Cloning remote Adafruit repository");
      //cloned_libraries is false if they already existed...if false, let's pull the latest version of the adafruit libraries
      if (!cloned_libraries) {
        git_helper.pull(config.adafruit.repository, "origin", "master", function() {
          ws_helper.send_message(ws, "self-check-message", "Adafruit repository updated");
        });
      }
    });
  };

  exports.health_check = function(ws) {
    //TODO redis to nedb
    db.findOne({type: "editor:settings"}, function(err, settings) {
        if (settings) {
          settings.offline = true;
        } else {
          settings = {offline: true};
        }
        winston.debug("getting settings", settings);
        ws_helper.send_message(ws, "self-check-settings", settings);
    })

    this.setup_adafruit_libraries(ws);

    git_helper.set_config(function() {
      var my_repository = "my-pi-projects";
      fs_helper.check_for_repository(my_repository, function(err, exists) {
        ws_helper.send_message(ws, "self-check-message", "Validated my-pi-projects");
        winston.debug(exists);
        if (exists) {
          git_helper.pull(my_repository, "origin", "master", function(err, status) {
            ws_helper.send_message(ws, "self-check-message", "Updated my-pi-projects");
            ws_helper.send_message(ws, 'self-check-complete');
          });
        } else {
          git_helper.create_local_repository(my_repository, function(err, response) {
            ws_helper.send_message(ws, "self-check-message", "Cloned my-pi-projects on local system");
            fs_helper.create_project_gitignore(function(err, file) {
              ws_helper.send_message(ws, "self-check-message", "Added .gitignore in my-pi-projects");
              fs_helper.create_project_readme(function(err, file) {
                ws_helper.send_message(ws, "self-check-message", "Added README.md in my-pi-projects");
                winston.debug(file);
                if (err) winston.error(err);

                git_helper.commit_push_and_save(file, "Added README.md and .gitignore", function(err, response) {
                  ws_helper.send_message(ws, "self-check-message", "Pushed changes to my-pi-projects");
                  ws_helper.send_message(ws, 'self-check-complete');
                });
              });
            });
          });
        }
        winston.debug("after everything");
      });
    });
  };
