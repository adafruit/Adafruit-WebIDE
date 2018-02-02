var path = require('path'),
    db = require('../models/webideModel'),
    exec = require('child_process').exec,
    fs = require ('fs'),
    winston = require('winston'),
    exec_helper = require('./exec_helper'),
    later = require('later').later;
    enParser = require('later').enParser,
    job_queue = [];

  fs.exists || (fs.exists = path.exists);

function execute_job(file) {
  var file_path = path.resolve(__dirname + "/../" + file.path.replace('\/filesystem\/', '\/repositories\/'));

  fs.exists(file_path, function(exists) {
    if (exists) {
      exec_helper.execute_program(file, true);
      console.log("execute_job");
      console.log(file.key);

      //TODO redis to nedb
      //client.hmset(file.key, "last_run", new Date(), function() {
        //repopulate the job list in the editor
      //});
    } else {
      winston.info('scheduled job no longer exists, deleting from queue: ' + file_path);
      //TODO redis to nedb
      //client.del(file.key);
      //client.srem("jobs", file.key);
    }
  });

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

      //console.log(job_queue);
}


exports.emit_scheduled_jobs = function emit_scheduled_jobs(username, socket) {
  var job_list = [];
  //TODO redis to nedb
  // client.smembers("jobs", function(err, res) {
  //   res.forEach(function(key, i) {
  //     client.hgetall(key, function(err, job_data) {
  //       if (job_data.username === username) {
  //         job_list.push(job_data);
  //       }
  //
  //       if (res.length === (i+1)) {
  //         socket.emit('scheduled-job-list', job_list);
  //       }
  //     });
  //   });
  // });
};

/*
 * Create new schedule
 */
exports.add_schedule = function(schedule, socket, session) {
  var self = this;
  schedule.file.username = session.username;
  var file_path = schedule.file.path.replace('\/filesystem\/', '\/repositories\/');
  var key = "jobs:" + file_path.replace(/\W/g, '');  //keep only alphanumeric for key
  var job_data = {
      text: schedule.text,
      name: schedule.file.name,
      key: key,
      last_run: "",
      active: "1",
      path: file_path,
      extension: schedule.file.extension,
      username: schedule.file.username
  };
  console.log("add_schedule");
  console.log(job_data);

  //TODO redis to nedb
  // client.sadd("jobs", key, function() {
  //   client.hmset(key, job_data, function() {
  //     schedule_job(key, job_data);
  //     //repopulate the job list in the editor
  //     self.emit_scheduled_jobs(session.username, socket);
  //   });
  // });
};

exports.delete_job = function(key, socket, session) {
  var self = this;
  var len = job_queue.length;
  for (var i=0; i<len; i++) {
    if (job_queue[i].key === key) {
      //job exists, lets delete it
      job_queue[i].later.stopExec();
      //remove from array
      job_queue.splice(i, 1);
      //remove from redis
      //TODO redis to nedb
      // client.del(key);
      // client.srem("jobs", key);
      //emit change to front-end
      self.emit_scheduled_jobs(session.username, socket);
      break;
    }
  }
};

exports.toggle_job = function(key, socket, session) {
  var self = this;
  console.log(key);
  //TODO redis to nedb
  // client.hgetall(key, function(err, job) {
  //   console.log(job);
  //   //toggle status
  //   job.active = 1-job.active;
  //
  //   client.hmset(key, "active", job.active, function() {
  //     if (!job.active) {
  //       //remove job from queue, but not redis
  //       var len = job_queue.length;
  //       for (var i=0; i<len; i++) {
  //         if (job_queue[i].key === key) {
  //           //job exists, lets delete it
  //           job_queue[i].later.stopExec();
  //           //remove from array
  //           job_queue.splice(i, 1);
  //           //emit change to front-end
  //           self.emit_scheduled_jobs(session.username, socket);
  //           break;
  //         }
  //       }
  //     } else {
  //       schedule_job(key, job);
  //       //repopulate the job list in the editor
  //       self.emit_scheduled_jobs(session.username, socket);
  //     }
  //
  //
  //   });
  // });

};

/*
 * Jobs initialized at server startup
 */
exports.initialize_jobs = function() {
  //TODO redis to nedb
  // client.smembers("jobs", function(err, res) {
  //   res.forEach(function(key) {
  //     client.hgetall(key, function(err, job_data) {
  //       schedule_job(key, job_data);
  //     });
  //   });
  // });
};
