var fs_helper = require('../helpers/fs_helper'),
    path = require('path'),
    redis = require('redis'),
    client = redis.createClient(),
    git_helper = require('../helpers/git_helper'),
    config = require('../config/config'),
    check = require('validator').check,
    sanitize = require('validator').sanitize;

//Loads the editor
exports.index = function(req, res) {
  var shown_notification = false;
  client.get('editor:shown_notification', function(err, result) {
    if (result) {
      shown_notification = result;
    } else {
      client.set('editor:shown_notification', true);
    }
    res.render('editor/index', {profile: req.user, version: config.editor.version, shown_notification: shown_notification});
  });
};

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