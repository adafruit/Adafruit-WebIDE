var express = require('express'),
    tty = require('tty.js'),
    app = express(),
    util = require('util'),
    io = require('socket.io'),
    passport = require('passport'),
    util = require('util'),
    BitbucketStrategy = require('passport-bitbucket').Strategy,
    GitHubStrategy = require('passport-github').Strategy,
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
    RedisStore = require('connect-redis')(express),
    redis = require("redis"),
    client = redis.createClient(),
    config = require('./config/config'),
    winston = require('winston');

var davServer,
    HOSTNAME,
    IS_PASSPORT_SETUP = false,
    REPOSITORY_PATH = path.resolve(__dirname + "/repositories");

winston.info("REPOSITORY_PATH", REPOSITORY_PATH);

//exec_helper.spawn_ipython();

//check for the existence of the logs directory, if it doesn't
//exist, create it prior to starting the child process.
var exists = path.existsSync(__dirname + '/logs');
if (!exists) {
  fs.mkdirSync(__dirname + '/logs', 0755);
  winston.info('created logs folder');
}

winston.add(winston.transports.File, { filename: __dirname + '/logs/output.log', json: false });
winston.handleExceptions(new winston.transports.File({ filename: __dirname + '/logs/errors.log', json: false }));
winston.remove(winston.transports.Console);

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
function setup_bitbucket_passport(consumer_key, consumer_secret) {
  winston.info("http://" + HOSTNAME + "/auth/bitbucket/callback");
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

// Use the GitHubStrategy within Passport.
//   Strategies in passport require a `verify` function, which accept
//   credentials (in this case, a token, tokenSecret, and Github profile),
//   and invoke a callback with a user object.
function setup_github_passport(consumer_key, consumer_secret) {
  winston.info("http://" + HOSTNAME + "/auth/github/callback");
  passport.use(new GitHubStrategy({
      clientID: consumer_key,
      clientSecret: consumer_secret,
      callbackURL: "http://" + HOSTNAME + "/auth/github/callback"
    },
    function(accessToken, refreshToken, profile, done) {
      // asynchronous verification, for effect...
      process.nextTick(function () {
        profile.token = accessToken;
        profile.refresh_token = refreshToken;
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
  //res.header('Cache-Control', 'no-cache, private, no-store, must-revalidate, max-stale=0, post-check=0, pre-check=0');
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
//app.use(express.logger("dev"));
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

app.use(errorHandler);


app.get('/', ensureAuthenticated, site.index);

app.get('/editor', ensureAuthenticated, editor.index);
app.get('/editor/image', ensureAuthenticated, editor.image);
app.post('/editor/upload', ensureAuthenticated, editor.upload_file);

app.post('/create/repository', ensureAuthenticated, editor.create_repository);

app.get('/setup', user.setup);
app.post('/setup', user.submit_setup);
app.get('/config', user.config);
app.post('/config', user.submit_config);
app.get('/set-datetime', user.set_datetime);
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

// GET /auth/github
app.get('/auth/github',
  passport.authenticate('github'),
  function(req, res){
    // The request will be redirected to Github for authentication, so this
    // function will not be called.
  });

// GET /auth/github/callback
app.get('/auth/github/callback',
  passport.authenticate('github', { failureRedirect: '/login' }),
  function(req, res) {
    res.redirect('/editor');
  });

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

// Simple route middleware to ensure user is authenticated.
//   Use this route middleware on any resource that needs to be protected.  If
//   the request is authenticated (typically via a persistent login session),
//   the request will proceed.  Otherwise, the user will be redirected to the
//   login page.
function ensureAuthenticated(req, res, next) {
  setHostName(req);

  if (config.editor.offline) {
    req.user = { provider: 'offline',
                 username: 'offline user' };
    return next();
  }

  function authRoute(req, res, next) {
    if (req.isAuthenticated()) {
      return next();
    }

    if (!IS_PASSPORT_SETUP) {
      res.redirect('/setup');
    } else {
      res.redirect('/login');
    }
  }

  //use the correct key for redis, either github or bitbucket
  var oauth_key = 'bitbucket_oauth';
  if (config.editor.github) {
    oauth_key = 'github_oauth';
  }

  if (!IS_PASSPORT_SETUP) {
    //need to setup passport on server startup, if the oauth is already setup
    client.hgetall(oauth_key, function (err, oauth) {
      if (oauth) {
        if (config.editor.github) {
          setup_github_passport(oauth.consumer_key, oauth.consumer_secret);
        } else {
          setup_bitbucket_passport(oauth.consumer_key, oauth.consumer_secret);
        }
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
    req.user = { provider: 'offline',
                 username: 'offline user' };
    return next();
  }

  //use the correct key for redis, either github or bitbucket
  var oauth_key = 'bitbucket_oauth';
  if (config.editor.github) {
    oauth_key = 'github_oauth';
  }

  client.hgetall(oauth_key, function (err, oauth) {
    if (!oauth) {
      res.redirect('/setup');
    } else {
      if (config.editor.github) {
        setup_github_passport(oauth.consumer_key, oauth.consumer_secret);
      } else {
        setup_bitbucket_passport(oauth.consumer_key, oauth.consumer_secret);
      }

      if (!IS_PASSPORT_SETUP) {
        IS_PASSPORT_SETUP = true;
        res.redirect('/login');
      } else {
        next();
      }
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
    if (!path.existsSync("/home/webide/repositories")) {
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
  io = io.listen(server);
  io.configure(function() {
    io.enable('browser client minification');
    io.enable('browser client etag');
    io.set('transports', ['websocket', 'htmlfile', 'xhr-polling', 'jsonp-polling']);
  });

  new tty.Server(config.term, app, server, io);

  client.hgetall('server', function (err, server_data) {
    var port;

    if (server_data && server_data.port) {
      port = server_data.port;
    } else {
      port = config.editor.port;
    }

    if (server_data && server_data.offline) {
      config.editor.offline = (server_data.offline == 1) ? true : false;
    }

    if (server_data && server_data.github) {
      config.editor.github = (server_data.github == 1) ? true : false;
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
        handshakeData.session = { provider: 'offline', username: 'offline user' };
        return callback(null, true);
      } else {
        client.get(session.passport.user, function(err, user) {
          if (err || !session) return callback('socket.io: session not found.', false);
          handshakeData.session = JSON.parse(user);
          if (handshakeData.session) {
            return callback(null, true);
          } else {
            return callback('socket.io: session user not found', false);
          }
        });
      }
    });
  });

  io.sockets.on('connection', function (socket) {
    socket.set('username', socket.handshake.session.username);

    //emit on first connection
    socket.emit('cwd-init', {dirname: REPOSITORY_PATH});
    scheduler.emit_scheduled_jobs(socket.handshake.session.username, socket);

    socket.on('disconnect', function() {
      debug_helper.client_disconnect();
      debug_helper.kill_debug();
    });

    //listen for events
    socket.on('git-delete', function(data) {
      git_helper.remove_commit_push(data.file, socket.handshake.session, function(err, status) {
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

      git_helper.commit_push_and_save(data.file, commit_message, socket.handshake.session, function(err, status) {
        socket.emit('commit-file-complete', {err: err, status: status});
      });
    });

    socket.on('move-file', function (data) {
      git_helper.move_commit_push(data.file, socket.handshake.session, function(err) {
        console.log('move-file', err);
        socket.emit('move-file-complete', {err: err});
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
        data.file.username = socket.handshake.session.username;
      }

      exec_helper.execute_program(data.file, false);
      git_helper.commit_push_and_save(data.file, "Modified " + data.file.name, socket.handshake.session, function(err, status) {
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

    socket.on('set-settings', function(value) {
      client.hmset("editor:settings", value, function(err) {
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