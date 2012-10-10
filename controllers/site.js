var request_helper = require('../helpers/request_helper');

exports.index = function(req, res){
  if (req.user) {
    res.redirect('/editor');
  }
};