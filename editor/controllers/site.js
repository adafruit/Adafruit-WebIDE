var request_helper = require('../helpers/request_helper');

exports.index = function(req, res){
  console.log(req.user);

  if (req.user) {
    res.redirect('/editor');
  }
};