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
    editor_setup = require('./helpers/editor_setup'),
    git_helper = require('./helpers/git_helper'),
    fs_helper = require('./helpers/fs_helper'),
    request_helper = require('./helpers/request_helper'),
    RedisStore = require('connect-redis')(express),
    redis = require("redis"),
    client = redis.createClient(),
    config = require('./config/config');

var davServer;
var REPOSITORY_PATH = path.resolve(__dirname + "/../repositories");
console.log("REPOSITORY_PATH", REPOSITORY_PATH);

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
  passport.serializeUser(function(user, done) {
    //console.log("serializeUser");
    //console.log(user);
    client.set(user.username, JSON.stringify(user));
    done(null, user.username);
  });

  passport.deserializeUser(function(obj, done) {
    //console.log("deserializeUser");
    //console.log(obj);
    client.get(obj, function(err, reply) {
      //console.log(JSON.parse(reply));
      done(null, JSON.parse(reply));
    });
  });

  passport.use(new BitbucketStrategy({
      consumerKey: consumer_key,
      consumerSecret: consumer_secret,
      callbackURL: "http://raspberrypi.local:3000/auth/bitbucket/callback"
      //callbackURL: "http://127.0.0.1:3000/auth/bitbucket/callback"
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

//need to setup passport on server startup, if the bitbucket oauth is already setup
client.hgetall('bitbucket_oauth', function (err, bitbucket) {
  if (bitbucket) {
    setup_passport(bitbucket.consumer_key, bitbucket.consumer_secret);
  }
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
  if (config.editor.offline) {
    //TODO: create a dummy session here
    return next();
  }
  if (req.isAuthenticated()) {
    return next();
  }

  client.hgetall('bitbucket_oauth', function (err, bitbucket) {
    if (!bitbucket) {
      res.redirect('/setup');
    } else {
      res.redirect('/login');
    }
  });
}

function ensureOauth(req, res, next) {
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

function serverInitialization(app) {

  var exists = path.existsSync(REPOSITORY_PATH);
  if (!exists) {
    fs.mkdirSync(REPOSITORY_PATH, 0777);
    console.log('created repositories folder');
  }

  var server = start_server();
  socket_listeners();
  mount_dav(server);
}

function start_server() {
  console.log('listening on port 3000');

  server = require('http').createServer(app);
  io = io.listen(server);
  new tty.Server(config.term, app, server, io);

  return server.listen(3000);
}

function socket_listeners() {
  io.sockets.authorization(function(handshakeData, callback) {
    if (!handshakeData.headers.cookie) return callback('socket.io: cookie not found.', false);
    var signedCookies = require('express/node_modules/cookie').parse(handshakeData.headers.cookie);
    handshakeData.cookies = require('express/node_modules/connect/lib/utils').parseSignedCookies(signedCookies, 'cat nap');

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
    socket.emit('cwd-init', {dirname: __dirname + '/../repositories'});

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
  });
}

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