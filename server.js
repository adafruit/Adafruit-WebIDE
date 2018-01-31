var express = require('express'),
    session = require('express-session'),
    bodyParser = require('body-parser'),
    methodOverride = require('method-override'),
    cookieParser = require('cookie-parser'),
    serveStatic = require('serve-static'),
    morgan = require('morgan'),
    app = express(),
    util = require('util'),
    io = require('socket.io'),
    util = require('util'),
    site = require('./controllers/site'),
    editor = require('./controllers/editor'),
    user = require('./controllers/user'),
    jsDAV = require("jsDAV/lib/jsdav"),
    fs = require('fs'),
    path = require('path'),
    updater = require('./helpers/updater'),
    scheduler = require('./helpers/scheduler'),
    editor_setup = require('./helpers/editor_setup'),
    git_helper = require('./helpers/git_helper'),
    exec_helper = require('./helpers/exec_helper'),
    fs_helper = require('./helpers/fs_helper'),
    exec_helper = require('./helpers/exec_helper'),
    request_helper = require('./helpers/request_helper'),
    debug_helper = require('./helpers/python/debug_helper'),
    config = require('./config/config'),
    winston = require('winston'),
    Datastore = require('nedb');

var davServer,
    HOSTNAME,
    REPOSITORY_PATH = path.resolve(__dirname + "/repositories");

winston.info("REPOSITORY_PATH", REPOSITORY_PATH);

//exec_helper.spawn_ipython();

//check for the existence of the logs directory, if it doesn't
//exist, create it prior to starting the child process.
var exists = fs.existsSync(__dirname + '/logs');
if (!exists) {
  fs.mkdirSync(__dirname + '/logs', 0755);
  winston.info('created logs folder');
}

//winston.add(winston.transports.File, { filename: __dirname + '/logs/output.log', json: false });
//winston.handleExceptions(new winston.transports.File({ filename: __dirname + '/logs/errors.log', json: false }));
//winston.remove(winston.transports.Console);


var db = new Datastore({ filename: './database/webide_data_store', autoload: true });

//redirect anything with /filesystem in the url to the WebDav server.
app.use(function(req, res, next) {
  //res.header('Cache-Control', 'no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0');
  if (req.path.indexOf("/filesystem") != -1) {
    davServer.exec(req, res);
  } else {
    next();
  }
});

app.set('view engine', 'jade');
app.set('views', __dirname + '/views');
//logging
app.use(morgan());
app.use(serveStatic(__dirname + '/public'));
app.use(serveStatic(__dirname + '/node_modules/xterm/dist'));
app.use(cookieParser());
var sessionMiddleware = session({
  key: 'sid',
  secret: 'cat nap',
  resave: true,
  saveUninitialized: true
});
app.use(sessionMiddleware);
app.use(bodyParser.json())
app.use(bodyParser.urlencoded({ extended: true }))
app.use(methodOverride());

app.use(function(req, res, next) {
  res.locals.session = req.session;
  next();
});

app.get('/', site.index);

app.get('/editor', editor.index);
app.get('/editor/image', editor.image);
app.post('/editor/upload', editor.upload_file);

app.post('/create/repository', editor.create_repository);

app.get('/setup', user.setup);
app.post('/setup', user.submit_setup);
app.get('/config', user.config);
app.post('/config', user.submit_config);
app.get('/set-datetime', user.set_datetime);

app.use(errorHandler);

serverInitialization(app);


function errorHandler(err, req, res, next) {
  winston.error(err);
  if (err.name === "InternalOAuthError") {
    res.status(500);
    res.render('oauth_error', { error: err });
  } else {
    res.status(500);
    res.render('error', { error: err });
  }
}

function setHostName(req) {
  //set it each time, it's quick, and hostname may change (internal IP vs external IP).
  HOSTNAME = req.headers.host;
}

function serverInitialization(app) {

  //setup repositories path
  var exists = fs.existsSync(REPOSITORY_PATH);
  if (!exists) {
    fs.mkdirSync(REPOSITORY_PATH, 0777);
    winston.info('created repositories folder');
  }

  //setup symlink to webide home, if it exists:
  var has_webide_path = fs.existsSync("/home/webide");
  if (has_webide_path) {
    //Creating symbolic link to repositories path
    winston.info('Linked repository paths: /home/webide/repositories');
    if (!fs.existsSync("/home/webide/repositories")) {
      fs.symlinkSync(REPOSITORY_PATH, "/home/webide/repositories", 'dir');
    }
  }

  scheduler.initialize_jobs();

  start_server(function(server) {
    socket_listeners();
    mount_dav(server);
  });
}

function start_server(cb) {
  server = require('http').createServer(app);
  io = io(server);
  io.use(function(socket, next) {
    sessionMiddleware(socket.request, socket.request.res, next);
  });

  db.find('server', function (err, server_data) {
    var port;

    if (server_data && server_data.port) {
      port = server_data.port;
    } else {
      port = config.editor.port;
    }

    winston.info('listening on port ' + port);
    cb(server.listen(port));
  });
}

function socket_listeners() {

  io.on('connection', function (socket) {
    winston.debug('socket io connection completed');
    //socket.set('username', socket.request.session.username);
    winston.debug("after username set");

    //emit on first connection
    socket.emit('cwd-init', {dirname: REPOSITORY_PATH});
    scheduler.emit_scheduled_jobs(socket.request.session.username, socket);

    socket.on('disconnect', function() {
      debug_helper.client_disconnect();
      debug_helper.kill_debug();
    });

    //listen for events
    socket.on('git-delete', function(data) {
      git_helper.remove_commit_push(data.file, socket.request.session, function(err, status) {
        socket.emit('git-delete-complete', {err: err, status: status});
      });
    });

    //listen for events
    socket.on('git-pull', function(data) {
      console.log(data);
      var name = data.file ? data.file.name : "";
      git_helper.pull(name, "origin", "master", function(err, status) {
        socket.emit('git-pull-complete', {err: err, status: status});
      });
    });

    //listen for events
    socket.on('git-is-modified', function(data) {
      git_helper.is_modified(data.file, function(err, status) {
        socket.emit('git-is-modified-complete', {is_modified: status});
      });
    });

    socket.on('commit-file', function (data) {
      var commit_message = "";

      if (data.message) {
        commit_message = data.message;
      } else {
        commit_message = "Modified " + data.file.name;
      }

      git_helper.commit_push_and_save(data.file, commit_message, socket.request.session, function(err, status) {
        socket.emit('commit-file-complete', {err: err, status: status});
      });
    });

    socket.on('move-file', function (data) {
      git_helper.move_commit_push(data.file, socket.request.session, function(err) {
        console.log('move-file', err);
        socket.emit('move-file-complete', {err: err});
      });
    });

    socket.on('self-check-request', function() {
      winston.debug('self-check-request');
      editor_setup.health_check(socket, socket.request.session);
    });

    socket.on('editor-check-updates', function() {
      updater.check_for_updates(socket);
    });

    socket.on('editor-update', function() {
      updater.update(socket);
    });

    socket.on('trace-file', function(data) {
      exec_helper.trace_program(data.file, socket);
    });

    socket.on('debug-command', function(data) {
      debug_helper.debug_command(data, socket);
    });

    socket.on('debug-file', function(data) {
      debug_helper.start_debug(data.file, socket);
    });

    socket.on('commit-run-file', function(data) {
      if (data && data.file) {
        data.file.username = socket.request.session.username;
      }

      exec_helper.execute_program(data.file, false);
      git_helper.commit_push_and_save(data.file, "Modified " + data.file.name, socket.request.session, function(err, status) {
        socket.emit('commit-file-complete', {message: "Save was successful"});
      });
    });

    socket.on('stop-script-execution', function(data) {
      exec_helper.stop_program(data.file, false);
    });

    socket.on('submit-schedule', function(schedule) {
      scheduler.add_schedule(schedule, socket, socket.request.session);
    });

    socket.on('schedule-delete-job', function(key) {
      scheduler.delete_job(key, socket, socket.request.session);
    });

    socket.on('schedule-toggle-job', function(key) {
      scheduler.toggle_job(key, socket, socket.request.session);
    });

    socket.on('set-settings', function(value) {
      value["type"] = "editor:settings";
      db.update({type: "editor:settings"}, value, { upsert: true }, function(err) {
        if (err) winston.error(err);
      });
    });
  });
}

io.sockets.on('disconnect', function(socket) {
  exec_helper.set_sockets(io.sockets.sockets);
});

function mount_dav(server) {
  var jsDAV_Tree_Filesystem = require("jsDAV/lib/DAV/tree/filesystem").jsDAV_Tree_Filesystem;
  //jsDAV.debugMode = true;
  davServer = jsDAV.mount({
    path: REPOSITORY_PATH,
    mount: '/filesystem',
    plugins: ["codesearch", "tree", "filelist", "filesearch", "locks", "mount", "temporaryfilefilter"],
    server: server,
    standalone: false,
    tree: new jsDAV_Tree_Filesystem(REPOSITORY_PATH)
  });
  winston.info('webdav filesystem mounted');
}

exports.get_socket = function (username, cb) {
  for (var socketId in io.sockets.sockets) {
    io.sockets.sockets[socketId].get('username', function(err, sock_username) {
      if (username === sock_username) {
        cb(io.sockets.sockets[socketId]);
      }
    });
  }
};

process.on('SIGINT', function() {
  winston.info("\nShutting down from  SIGINT");
  // some other closing procedures go here
  debug_helper.kill_debug(false, function() {
    //no need to wait for this
  });
  process.exit();
});

process.on('uncaughtException', function(err) {
  debug_helper.kill_debug(false, function() {
    //no need to wait for this
  });
});
