var exec = require('child_process').exec,
  fs = require ('fs'),
  path = require('path'),
  exec_helper = require('./exec_helper'),
  redis = require('redis'),
  client = redis.createClient(),
  later = require('later').later;
  enParser = require('later').enParser;

  fs.exists || (fs.exists = path.exists);

function execute_job(file) {
  exec_helper.execute_program(file, true);
  //console.log(file);
}

function schedule_job(job) {
      var l = later(60);
      var schedule = enParser().parse(job.text);
      l.exec(schedule, new Date(), execute_job, job.file);  
      console.log("Job Scheduled: ", schedule);
}
/*
 * Create new schedule
 */
exports.add_schedule = function (schedule, socket, session) {
  schedule.file.username = session.username;
  var file_path = schedule.file.path.replace('\/filesystem\/', '\/repositories\/');
  var key = "jobs:" + file_path.replace(/\W/g, '');  //keep only alphanumeric for key
  client.sadd("jobs", key, function() {
    client.hmset(key, {
      text: schedule.text,
      name: schedule.file.name,
      path: file_path,
      username: schedule.file.username
    }, function() {
      schedule_job(schedule);
    });
  });
};



/*
 * Jobs initialized at server startup
 */
exports.initialize_jobs = function() {
  client.smembers("jobs", function(err, res) {
    res.forEach(function(job) {
      console.log(job);
      client.hgetall(job, function(err, job_data) {
        schedule_job(job_data);
      });
    });
  });
};