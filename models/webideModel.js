'use strict';

var Datastore = require('nedb'),
    path = require('path');

var webideDb = new Datastore({
  filename: path.resolve(__dirname, '../db/webide_data_store'),
  autoload: true
});

module.exports = webideDb;
