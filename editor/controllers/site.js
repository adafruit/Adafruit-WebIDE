var request_helper = require('../helpers/request_helper');

exports.index = function(req, res){
  console.log(req.user);

  if (req.user) {
    //request_helper.post_ssh_key(req.user, function() {
    //  res.render('index', { title: 'Home', user: req.user });
    //});
    res.redirect('/editor');
  }
};