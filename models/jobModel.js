'use strict';

var Datastore = require('nedb'),
    path = require('path');

var jobsDb = new Datastore({
  filename: path.resolve(__dirname, '../db/jobs_data_store'),
  autoload: true
});

module.exports = jobsDb;
