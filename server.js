var express = require('express'),
    session = require('express-session'),
    bodyParser = require('body-parser'),
    methodOverride = require('method-override'),
    cookieParser = require('cookie-parser'),
    serveStatic = require('serve-static'),
    morgan = require('morgan'),
    app = express(),
    expressWs = require('express-ws')(app),
    multer  = require('multer'),
    upload = multer({ dest: './uploads' }),
    util = require('util'),
    util = require('util'),
    site = require('./controllers/site'),
    editor = require('./controllers/editor'),
    user = require('./controllers/user'),
    jsDAV = require("jsDAV/lib/jsdav"),
    fs = require('fs'),
    path = require('path'),
    scheduler = require('./helpers/scheduler'),
    fs_helper = require('./helpers/fs_helper'),
    debug_helper = require('./helpers/python/debug_helper'),
    config = require('./config/config'),
    winston = require('winston'),
    db = require('./models/webideModel'),
    pty = require('node-pty');

var davServer,
    HOSTNAME,
    REPOSITORY_PATH = path.resolve(__dirname + "/repositories");



var terminals = {}, logs = {};

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


winston.info("REPOSITORY_PATH", REPOSITORY_PATH);

//redirect anything with /filesystem in the url to the WebDav server.
app.use(function(req, res, next) {
  //res.header('Cache-Control', 'no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0');
  if (req.path.indexOf("/filesystem") != -1) {
    davServer.exec(req, res);
  } else {
    next();
  }
});

app.set('view engine', 'pug');
app.set('views', __dirname + '/views');
//logging
app.use(morgan('tiny'));
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

app.post('/editor/upload', upload.single('obj'), editor.upload_file);

app.post('/create/repository', editor.create_repository);

app.get('/setup', user.setup);
app.post('/setup', user.submit_setup);
app.get('/config', user.config);
app.post('/config', user.submit_config);
app.get('/set-datetime', user.set_datetime);

app.post('/terminals', function (req, res) {
  var cols = parseInt(req.query.cols),
      rows = parseInt(req.query.rows),
      cwd = req.query.cwd;

  var term = pty.spawn(process.platform === 'win32' ? 'cmd.exe' : 'bash', [], {
        name: 'xterm-color',
        cols: cols || 80,
        rows: rows || 24,
        cwd: path.resolve(cwd),
        env: process.env
      });

  console.log('Created terminal with PID: ' + term.pid);
  terminals[term.pid] = term;
  logs[term.pid] = '';
  term.on('data', function(data) {
    logs[term.pid] += data;
  });
  res.send(term.pid.toString());
  res.end();
});

app.post('/terminals/:pid/size', function (req, res) {
  var pid = parseInt(req.params.pid),
      cols = parseInt(req.query.cols),
      rows = parseInt(req.query.rows),
      term = terminals[pid];

  term.resize(cols, rows);
  console.log('Resized terminal ' + pid + ' to ' + cols + ' cols and ' + rows + ' rows.');
  res.end();
});

app.ws('/terminals/:pid', function (ws, req) {
  var term = terminals[parseInt(req.params.pid)];
  console.log('Connected to terminal ' + term.pid);
  ws.send(logs[term.pid]);

  term.on('data', function(data) {
    try {
      ws.send(data);
    } catch (ex) {
      // The WebSocket is not open, ignore
    }
  });
  ws.on('message', function(msg) {
    try {
      msg = JSON.parse(msg);
    } catch (e) {
      //not json, just a string...
    }

    if (msg.type === 'input') {
      term.write(msg.data.toString() + '\r');
    } else {
      term.write(msg.toString());
    }
  });
  ws.on('close', function () {
    term.kill();
    console.log('Closed terminal ' + term.pid);
    // Clean things up
    delete terminals[term.pid];
    delete logs[term.pid];
  });
});

app.ws('/editor', editor.editor);

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
    mount_dav(server);
  });
}

function start_server(cb) {
  db.find('server', function (err, server_data) {
    var port;

    if (server_data && server_data.port) {
      port = server_data.port;
    } else if (process.env.PORT) {
      port = process.env.PORT
    } else {
      port = config.editor.port;
    }



    winston.info('listening on port ' + port);
    cb(app.listen(port));
  });
}

function mount_dav(server) {
  var jsDAV_FS_Tree = require("jsDAV/lib/DAV/backends/fs/tree");
  //jsDAV.debugMode = true;
  davServer = jsDAV.mount({
    path: REPOSITORY_PATH,
    mount: '/filesystem',
    plugins: ["codesearch", "tree", "filelist", "filesearch", "locks", "mount", "temporaryfilefilter"],
    server: server,
    standalone: false,
    tree: jsDAV_FS_Tree.new(REPOSITORY_PATH)
  });
  winston.info('webdav filesystem mounted');
}

function cleanup() {
  winston.info("process and worker cleanup");
  Object.keys(terminals).forEach(function (pid) {
      var term = terminals[pid];
      console.log('Closed terminal ' + term.pid);
      term.kill();
  });

  debug_helper.kill_debug(false, function() {
    //no need to wait for this
  });
}

process.on('SIGINT', function() {
  winston.info("Shutting down from  SIGINT");
  cleanup();
  process.exit();
});

process.on('uncaughtException', function(err) {
  winston.error("Shutting down from uncaughtException");
  winston.error(err.stack);
  cleanup();
  process.exit();
});
