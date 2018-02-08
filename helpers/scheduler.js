var path = require('path'),
    db = require('../models/jobModel'),
    exec = require('child_process').exec,
    fs = require ('fs'),
    winston = require('winston'),
    exec_helper = require('./exec_helper'),
    ws_helper = require('./websocket_helper'),
    later = require('later'),
    job_queue = [];

  fs.exists || (fs.exists = path.exists);

function execute_job(file) {
  var file_path = path.resolve(__dirname + "/../" + file.path.replace('\/filesystem\/', '\/repositories\/'));

  fs.exists(file_path, function(exists) {
    if (exists) {
      exec_helper.execute_program(file, true);
      console.log("execute_job");
      console.log(file.key);

      file.last_run = new Date();
      db.update({key: file.key}, file, {upsert: true}, function(err) {

      });
    } else {
      winston.info('scheduled job no longer exists, deleting from queue: ' + file_path);
      //TODO redis to nedb
      db.remove({key: file.key});
    }
  });

}

function schedule_job(key, job) {
  if (!key) {
    return;
  }

  var is_new_job = true,
      schedule = later.parse.text(job.text);

  later.date.localTime();
  var job_timer = later.setInterval(execute_job.bind(null, job), schedule);
  console.log("Job Scheduled: ", schedule);

  var len = job_queue.length;
  for (var i=0; i<len; i++) {
    if (job_queue[i].key === key) {
      //job already exists, but has been modified, let's stop the existing one
      job_queue[i].job_timer.clear();

      //replace it in the queue
      job_queue[i] = {key: key, job_timer: job_timer};

      is_new_job = false;
      break;
    }
  }

  if (is_new_job) {
    job_queue.push({key: key, job_timer: job_timer});
  }

  //console.log(job_queue);
}


exports.emit_scheduled_jobs = function emit_scheduled_jobs(socket) {
  winston.debug("emit_scheduled_jobs");
  db.find({type: "job"}, function(err, data) {
    ws_helper.send_message(socket, 'scheduled-job-list', data);
  });
};

/*
 * Create new schedule
 */
exports.add_schedule = function(schedule, socket) {
  var self = this;
  var file_path = schedule.file.path.replace('\/filesystem\/', '\/repositories\/');
  var key = "jobs:" + file_path.replace(/\W/g, '');  //keep only alphanumeric for key
  var job_data = {
      type: "job",
      text: schedule.text,
      name: schedule.file.name,
      key: key,
      last_run: "",
      active: "1",
      path: file_path,
      extension: schedule.file.extension
  };
  console.log("add_schedule");
  console.log(job_data);

  db.update({key: key}, job_data, {upsert: true}, function(err, numReplaced, upsert) {
    schedule_job(key, job_data);
    //repopulate the job list in the editor
    self.emit_scheduled_jobs(socket);
  });
};

exports.delete_job = function(key, socket) {
  var self = this;
  var len = job_queue.length;
  for (var i=0; i<len; i++) {
    if (job_queue[i].key === key) {
      //job exists, lets delete it
      job_queue[i].later.stopExec();
      //remove from array
      job_queue.splice(i, 1);
      //remove from redis
      db.remove({key: key});
      //emit change to front-end
      self.emit_scheduled_jobs(socket);
      break;
    }
  }
};

exports.toggle_job = function(key, socket) {
  var self = this;
  console.log(key);
  db.findOne({key: key}, function(err, job) {
    job.active = 1-job.active;

    db.update({key: key}, job, function() {
      if (!job.active) {
        //remove job from queue, but not redis
        var len = job_queue.length;
        for (var i=0; i<len; i++) {
          if (job_queue[i].key === key) {
            //job exists, lets delete it
            job_queue[i].later.stopExec();
            //remove from array
            job_queue.splice(i, 1);
            //emit change to front-end
            self.emit_scheduled_jobs(socket);
            break;
          }
        }
      } else {
        schedule_job(key, job);
        //repopulate the job list in the editor
        self.emit_scheduled_jobs(socket);
      }
    });
  });
};

/*
 * Jobs initialized at server startup
 */
exports.initialize_jobs = function() {
  db.find({type: "job"}, function(err, data) {
    schedule_job(data.key, data);
  });
};
