var fs_helper = require('../helpers/fs_helper'),
    path = require('path'),
    git_helper = require('../helpers/git_helper'),
    config = require('../config/config'),
    check = require('validator').check,
    sanitize = require('validator').sanitize;

//Loads the editor
exports.index = function(req, res) {
  res.render('editor/index', {profile: req.user, version: config.editor.version});
};

exports.create_repository = function(req, res) {
  var repository_url = sanitize(req.body.repository_url).xss().trim();

  git_helper.clone_update_remote_push(req.user, repository_url, function(err, status) {
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
      git_helper.commit_push_and_save({path: folder_path + file_name}, function(err, status) {
        res.send(true, 200);
      });
    }
  });
};