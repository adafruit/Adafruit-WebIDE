var fs_helper = require('../helpers/fs_helper'),
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