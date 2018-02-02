var fs_helper = require('../helpers/fs_helper'),
    winston = require('winston'),
    editor_setup = require('../helpers/editor_setup'),
    path = require('path'),
    git_helper = require('../helpers/git_helper'),
    config = require('../config/config'),
    check = require('validator').check,
    sanitize = require('validator').sanitize;

var REPOSITORY_PATH = path.resolve(process.env.PWD, "/repositories")

//Loads the editor
exports.index = function(req, res) {
  //var shown_notification = false;
  //client.get('editor:shown_notification', function(err, result) {
  //  if (result) {
  //    shown_notification = result;
  //  } else {
  //    client.set('editor:shown_notification', true);
  //  }
    res.render('editor/index', {profile: req.user, version: config.editor.version, shown_notification: false});
  //});
};

exports.editor = function(ws, req) {
  winston.debug('socket io connection completed');
  //socket.set('username', socket.request.session.username);
  winston.debug("after username set");

  function send_message(type, data) {
    ws.send(JSON.stringify({type: type, data: data}));
  }

  //emit on first connection
  send_message('cwd-init', {dirname: REPOSITORY_PATH});

  ws.on('message', function(msg) {
    var message = JSON.parse(msg);
    var type = message.type;
    var data = message.data;

    switch (type) {
      case 'self-check-request':
        winston.debug('self-check-request');
        editor_setup.health_check(ws);
        winston.debug('here');
        break;
      case 'git-delete':
        git_helper.remove_commit_push(data.file, ws.request.session, function(err, status) {
          send_message('git-delete-complete', {err: err, status: status});
        });
        break;
      case 'git-pull':
        var name = data.file ? data.file.name : "";
        git_helper.pull(name, "origin", "master", function(err, status) {
          send_message('git-pull-complete', {err: err, status: status});
        });
        break;
      case 'git-is-modified':
        git_helper.is_modified(data.file, function(err, status) {
          send_message('git-is-modified-complete', {is_modified: status});
        });
        break;
      case 'commit-file':
        var commit_message = "";

        if (data.message) {
          commit_message = data.message;
        } else {
          commit_message = "Modified " + data.file.name;
        }

        git_helper.commit_push_and_save(data.file, commit_message, ws.request.session, function(err, status) {
          send_message('commit-file-complete', {err: err, status: status});
        });
        break;
      case 'move-file':
        git_helper.move_commit_push(data.file, ws.request.session, function(err) {
          console.log('move-file', err);
          send_message('move-file-complete', {err: err});
        });
        break;
      case 'editor-check-updates':
        updater.check_for_updates(ws);
        break;
      case 'editor-update':
        updater.update(ws);
        break;
      case 'trace-file':
        exec_helper.trace_program(data.file, ws);
        break;
      case 'debug-command':
        debug_helper.debug_command(data, ws);
        break;
      case 'debug-file':
        debug_helper.start_debug(data.file, ws);
        break;
      case 'commit-run-file':
        if (data && data.file) {
          data.file.username = ws.request.session.username;
        }

        exec_helper.execute_program(data.file, false);
        git_helper.commit_push_and_save(data.file, "Modified " + data.file.name, ws.request.session, function(err, status) {
          send_message('commit-file-complete', {message: "Save was successful"});
        });
        break;
      case 'stop-script-execution':
        exec_helper.stop_program(data.file, false);
        break;
      case 'submit-schedule':
        scheduler.add_schedule(schedule, ws, ws.request.session);
        break;
      case 'schedule-delete-job':
        scheduler.delete_job(key, ws, ws.request.session);
        break;
      case 'schedule-toggle-job':
        scheduler.toggle_job(key, ws, ws.request.session);
        break;
      case 'set-settings':
        value["type"] = "editor:settings";
        db.update({type: "editor:settings"}, value, { upsert: true }, function(err) {
          if (err) winston.error(err);
        });
        break;

    }
  });

  ws.on('disconnect', function() {
    debug_helper.client_disconnect();
    debug_helper.kill_debug();
  });
}

exports.create_repository = function(req, res) {
  var repository_url = sanitize(req.body.repository_url).xss().trim();
  var retain_remote = sanitize(req.body.retain_remote).xss().trim();

  git_helper.clone_update_remote_push(req.user, repository_url, retain_remote, function(err, status) {
    if (err) res.send(err, 404);
    else res.send(status, 200);
  });
};

//Opens an image clicked from the editor navigator
exports.image = function(req, res) {
  var temp_path = sanitize(req.query.path).xss().trim().replace('/filesystem/', '/repositories/');
  //strip basic attempted path traversals
  temp_path = temp_path.replace('..', '');

  fs_helper.open_image(temp_path, function(err, data) {
    res.send(data, 200);
  });

};

exports.upload_file = function(req, res) {
  console.log(req.files.files[0]);

  var temp_path = sanitize(req.files.files[0].path).xss().trim();
  var file_name = sanitize(req.files.files[0].name).xss().trim();
  file_name = file_name.replace(" ", "_");
  var folder_path = sanitize(req.body.path).xss().trim().replace('filesystem', 'repositories');

  var new_path = __dirname + '/..' + folder_path + file_name;
  new_path = path.resolve(new_path);

  fs_helper.move_uploaded_file(temp_path, new_path, function(err, status) {
    if (err) {
    res.send(false, 200);
    } else {
      var comment = "Uploaded new File.";
      git_helper.commit_push_and_save({path: folder_path + file_name}, comment, null, function(err, status) {
        res.send(true, 200);
      });
    }
  });
};
