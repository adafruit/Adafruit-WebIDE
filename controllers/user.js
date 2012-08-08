exports.login = function(req, res){
  res.render('users/login', { title: 'test', user: req.user });
};

exports.logout = function(req, res){
  req.logout();
  res.redirect('/');
};
