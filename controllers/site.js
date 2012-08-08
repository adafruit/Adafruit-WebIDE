exports.index = function(req, res){
  console.log(req.user._json.repositories);
  res.render('index', { title: 'Home', user: req.user });
};
