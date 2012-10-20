var exec = require('child_process').exec,
  fs = require ('fs'),
  path = require('path'),
  exec_helper = require('./exec_helper'),
  redis = require('redis'),
  client = redis.createClient(),
  later = require('later').later;
  enParser = require('later').enParser,
  job_queue = [];

  fs.exists || (fs.exists = path.exists);

function execute_job(file) {
  exec_helper.execute_program(file, true);
  //console.log(file);
}

function schedule_job(key, job) {
      var is_new_job = true,
          l = later(60),
          schedule = enParser().parse(job.text);
          
      l.exec(schedule, new Date(), execute_job, job);
      console.log("Job Scheduled: ", schedule);

      var len = job_queue.length;
      for (var i=0; i<len; i++) {
        if (job_queue[i].key === key) {
          //job already exists, but has been modified, let's stop the existing one
          job_queue[i].later.stopExec();

          //replace it in the queue
          job_queue[i] = {key: key, later: l};
          is_new_job = false;
          break;
        }
      }

      if (is_new_job) {
        job_queue.push({key: key, later: l});
      }
      
      console.log(job_queue);
}
/*
 * Create new schedule
 */
exports.add_schedule = function (schedule, socket, session) {
  schedule.file.username = session.username;
  var file_path = schedule.file.path.replace('\/filesystem\/', '\/repositories\/');
  var key = "jobs:" + file_path.replace(/\W/g, '');  //keep only alphanumeric for key
  var job_data = {
      text: schedule.text,
      name: schedule.file.name,
      path: file_path,
      extension: schedule.file.extension,
      username: schedule.file.username
  };
  client.sadd("jobs", key, function() {
    client.hmset(key, job_data, function() {
      schedule_job(key, job_data);
    });
  });
};



/*
 * Jobs initialized at server startup
 */
exports.initialize_jobs = function() {
  client.smembers("jobs", function(err, res) {
    res.forEach(function(key) {
      client.hgetall(key, function(err, job_data) {
        schedule_job(key, job_data);
      });
    });
  });
};