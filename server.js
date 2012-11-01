var express = require('express'),
    tty = require('tty.js'),
    app = express(),
    util = require('util'),
    io = require('socket.io'),
    passport = require('passport'),
    util = require('util'),
    BitbucketStrategy = require('passport-bitbucket').Strategy,
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
    request_helper = require('./helpers/request_helper'),
    RedisStore = require('connect-redis')(express),
    redis = require("redis"),
    client = redis.createClient(),
    config = require('./config/config'),
    winston = require('winston');

var davServer,
    HOSTNAME,
    IS_PASSPORT_SETUP = false,
    REPOSITORY_PATH = path.resolve(__dirname + "/repositories");

console.log("REPOSITORY_PATH", REPOSITORY_PATH);

//exec_helper.spawn_ipython();

//check for the existence of the logs directory, if it doesn't
//exist, create it prior to starting the child process.
var exists = path.existsSync(__dirname + '/logs');
if (!exists) {
  fs.mkdirSync(__dirname + '/logs', 0755);
  console.log('created logs folder');
}

winston.add(winston.transports.File, { filename: __dirname + '/logs/output.log', json: false });
winston.handleExceptions(new winston.transports.File({ filename: __dirname + '/logs/errors.log', json: false }));
winston.info('Logger initialized!');
//winston.remove(winston.transports.Console);

// Passport session setup.
//   To support persistent login sessions, Passport needs to be able to
//   serialize users into and deserialize users out of the session.  Typically,
//   this will be as simple as storing the user ID when serializing, and finding
//   the user by ID when deserializing.  However, since this example does not
//   have a database of user records, the complete Bitbucket profile is
//   serialized and deserialized.


// Use the BitbucketStrategy within Passport.
//   Strategies in passport require a `verify` function, which accept
//   credentials (in this case, a token, tokenSecret, and Bitbucket profile),
//   and invoke a callback with a user object.
function setup_passport(consumer_key, consumer_secret) {

  passport.use(new BitbucketStrategy({
      consumerKey: consumer_key,
      consumerSecret: consumer_secret,
      callbackURL: "http://" + HOSTNAME + "/auth/bitbucket/callback"
    },
    function(token, tokenSecret, profile, done) {
      // asynchronous verification, for effect...
      process.nextTick(function () {
        profile.token = token;
        profile.token_secret = tokenSecret;
        profile.consumer_key = consumer_key;
        profile.consumer_secret = consumer_secret;

        return done(null, profile);
      });
    }
  ));
}

passport.serializeUser(function(user, done) {
  client.set(user.username, JSON.stringify(user));
  done(null, user.username);
});

passport.deserializeUser(function(obj, done) {
  client.get(obj, function(err, reply) {
    done(null, JSON.parse(reply));
  });
});

//redirect anything with /filesystem in the url to the WebDav server.
app.use(function(req, res, next) {
  if (req.path.indexOf("/filesystem") != -1) {
    davServer.exec(req, res);
  } else {
    next();
  }
});

var sessionStore = new RedisStore();
app.set('view engine', 'jade');
app.set('views', __dirname + '/views');
app.use(express.logger());
app.use(express.static(__dirname + '/public'));
app.use(express.static(__dirname + '/node_modules/tty.js/static'));
app.use(express.cookieParser());
app.use(express.session({
  store: sessionStore,
  key: 'sid',
  secret: 'cat nap'
}));
app.use(express.bodyParser());
app.use(express.methodOverride());
app.use(passport.initialize());
app.use(passport.session());

app.use(function(req, res, next) {
  res.locals.session = req.session;
  next();
});

app.use(app.router);


app.get('/', ensureAuthenticated, site.index);

app.get('/editor', ensureAuthenticated, editor.index);
app.get('/editor/image', ensureAuthenticated, editor.image);
app.post('/editor/upload', ensureAuthenticated, editor.upload_file);

app.post('/create/repository', ensureAuthenticated, editor.create_repository);

app.get('/setup', user.setup);
app.post('/setup', user.submit_setup);
app.get('/config', user.config);
app.post('/config', user.submit_config);
app.get('/login', ensureOauth, user.login);
app.get('/logout', user.logout);

// GET /auth/bitbucket
//   Use passport.authenticate() as route middleware to authenticate the
//   request.  The first step in Bitbucket authentication will involve redirecting
//   the user to bitbucket.org.  After authorization, Bitbucket will redirect the user
//   back to this application at /auth/bitbucket/callback
app.get('/auth/bitbucket',
  passport.authenticate('bitbucket'),
  function(req, res){
    // The request will be redirected to Bitbucket for authentication, so this
    // function will not be called.
  });

// GET /auth/bitbucket/callback
//   Use passport.authenticate() as route middleware to authenticate the
//   request.  If authentication fails, the user will be redirected back to the
//   login page.  Otherwise, the primary route function function will be calsled,
//   which, in this example, will redirect the user to the home page.
app.get('/auth/bitbucket/callback',
  passport.authenticate('bitbucket', { failureRedirect: '/login' }),
  function(req, res) {
    res.redirect('/editor');
  });

serverInitialization(app);




// Simple route middleware to ensure user is authenticated.
//   Use this route middleware on any resource that needs to be protected.  If
//   the request is authenticated (typically via a persistent login session),
//   the request will proceed.  Otherwise, the user will be redirected to the
//   login page.
function ensureAuthenticated(req, res, next) {
  setHostName(req);

  function authRoute(req, res, next) {
    if (config.editor.offline) {
      //TODO: create a dummy session here
      return next();
    }
    if (req.isAuthenticated()) {
      return next();
    }

    if (!IS_PASSPORT_SETUP) {
      res.redirect('/setup');
    } else {
      res.redirect('/login');
    }
  }

  if (!IS_PASSPORT_SETUP) {
    //need to setup passport on server startup, if the bitbucket oauth is already setup
    client.hgetall('bitbucket_oauth', function (err, bitbucket) {
      if (bitbucket) {
        setup_passport(bitbucket.consumer_key, bitbucket.consumer_secret);
        IS_PASSPORT_SETUP = true;
      }
      authRoute(req, res, next);
    });
  } else {
    authRoute(req, res, next);
  }

}

function ensureOauth(req, res, next) {
  setHostName(req);

  if (config.editor.offline) {
    //TODO: create a dummy session here
    return next();
  }

  client.hgetall('bitbucket_oauth', function (err, bitbucket) {
    if (!bitbucket) {
      res.redirect('/setup');
    } else {
      setup_passport(bitbucket.consumer_key, bitbucket.consumer_secret);
      return next();
    }
  });
}

function setHostName(req) {
  //set it each time, it's quick, and hostname may change (internal IP vs external IP).
  HOSTNAME = req.headers.host;
}

function serverInitialization(app) {

  //setup repositories path
  var exists = path.existsSync(REPOSITORY_PATH);
  if (!exists) {
    fs.mkdirSync(REPOSITORY_PATH, 0777);
    winston.info('created repositories folder');
  }

  //setup symlink to webide home, if it exists:
  var has_webide_path = path.existsSync("/home/webide");
  if (has_webide_path) {
    //Creating symbolic link to repositories path
    winston.info('Linked repository paths: /home/webide/repositories');
    fs.symlinkSync(REPOSITORY_PATH, "/home/webide/repositories", 'dir');
  }

  scheduler.initialize_jobs();

  start_server(function(server) {
    socket_listeners();
    mount_dav(server);
  });
}

function start_server(cb) {
  server = require('http').createServer(app);
  io = io.listen(server);
  new tty.Server(config.term, app, server, io);

  client.hgetall('server', function (err, server_data) {
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
  io.sockets.authorization(function(handshakeData, callback) {
    if (!handshakeData.headers.cookie) return callback('socket.io: cookie not found.', false);
    var signedCookies = require('express/node_modules/cookie').parse(handshakeData.headers.cookie);
    handshakeData.cookies = require('connect/lib/utils').parseSignedCookies(signedCookies, 'cat nap');

    sessionStore.get(handshakeData.cookies['sid'], function(err, session) {
      if (config.editor.offline) {
        return callback(null, true);
      }
      client.get(session.passport.user, function(err, user) {
        if (err || !session) return callback('socket.io: session not found.', false);
        handshakeData.session = JSON.parse(user);
        if (handshakeData.session) {
          return callback(null, true);
        } else {
          return callback('socket.io: session user not found', false);
        }
      });
    });
  });

  io.sockets.on('connection', function (socket) {
    socket.set('username', socket.handshake.session.username);

    //emit on first connection
    socket.emit('cwd-init', {dirname: REPOSITORY_PATH});
    scheduler.emit_scheduled_jobs(socket.handshake.session.username, socket);

    //listen for events
    socket.on('git-delete', function(data) {
      git_helper.remove_commit_push(data.file, function(err, status) {
        socket.emit('git-delete-complete', {message: ""});
      });
    });

    socket.on('commit-file', function (data) {
      git_helper.commit_push_and_save(data.file, function(err, status) {
        socket.emit('commit-file-complete', {message: "Save was successful"});
      });
    });

    socket.on('move-file', function (data) {
      git_helper.move_commit_push(data.file, function(err, status) {
        socket.emit('move-file-complete', {err: err, status: status});
      });
    });

    socket.on('self-check-request', function() {
      editor_setup.health_check(socket, socket.handshake.session);
    });

    socket.on('editor-check-updates', function() {
      updater.check_for_updates(socket);
    });

    socket.on('editor-update', function() {
      updater.update(socket);
    });

    socket.on('commit-run-file', function(data) {
      console.log(data);
      if (data && data.file) {
        data.file.username = socket.handshake.session.username;
      }

      exec_helper.execute_program(data.file, false);
      git_helper.commit_push_and_save(data.file, function(err, status) {
        socket.emit('commit-file-complete', {message: "Save was successful"});
      });
    });

    socket.on('stop-script-execution', function(data) {
      exec_helper.stop_program(data.file, false);
    });

    socket.on('submit-schedule', function(schedule) {
      scheduler.add_schedule(schedule, socket, socket.handshake.session);
    });

    socket.on('schedule-delete-job', function(key) {
      scheduler.delete_job(key, socket, socket.handshake.session);
    });

    socket.on('schedule-toggle-job', function(key) {
      scheduler.toggle_job(key, socket, socket.handshake.session);
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
  console.log('webdav filesystem mounted');
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