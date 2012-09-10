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
    git_helper = require('./helpers/git_helper'),
    request_helper = require('./helpers/request_helper'),
    exec_helper = require('./helpers/exec_helper'),
    RedisStore = require('connect-redis')(express),
    redis = require("redis"),
    client = redis.createClient(),
    config = require('./config');

var davServer;
console.log(__dirname);

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
      //callbackURL: "http://76.17.224.82:3000/auth/bitbucket/callback"
      //callbackURL: "http://127.0.0.1:3001/auth/bitbucket/callback"
    },
    function(token, tokenSecret, profile, done) {
      // asynchronous verification, for effect...
      process.nextTick(function () {
        profile.token = token;
        profile.token_secret = tokenSecret;
        profile.consumer_key = consumer_key;
        profile.consumer_secret = consumer_secret;

        //TODO REFACTOR THIS MESS
        request_helper.list_repositories(profile, function(err, list) {
          var exists = list.some(function(repository) {
            return (repository.name === config.adafruit.repository);
          });
          git_helper.clone_adafruit_libraries(config.adafruit.repository, config.adafruit.remote, function() {
            if (!exists) {
              request_helper.create_repository(profile, config.adafruit.repository, function(err, response) {
                console.log("created adafruit repository in bitbucket");
                git_helper.update_remote(profile, config.adafruit.repository, function(err, response) {
                  console.log("updated remote for adafruit repository");
                  git_helper.add_remote(config.adafruit.repository, config.adafruit.remote_name, config.adafruit.remote, function(err, response) {
                    console.log("added remote for adafruit repository");
                    git_helper.push(config.adafruit.repository, "origin", "master", function(err, response) {
                      return done(null, profile);
                    });
                  });
                });

              });
            } else {
              git_helper.update_remote(profile, config.adafruit.repository, function(err, response) {
                git_helper.add_remote(config.adafruit.repository, "adaremote", config.adafruit.remote, function(err, response) {
                  git_helper.push(config.adafruit.repository, "origin", "master", function(err, response) {
                    return done(null, profile);
                  });
                });
              });
            }
          });
        });

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

app.set('view engine', 'jade');
app.set('views', __dirname + '/views');
app.use(express.logger());
app.use(express.static(__dirname + '/public'));
app.use(express.static(__dirname + '/node_modules/tty.js/static'));
app.use(express.cookieParser());
app.use(express.session({
  store: new RedisStore(),
  key: 'sid',
  secret: 'cat nap'
}));
app.use(express.bodyParser());
app.use(express.methodOverride());
app.use(passport.initialize());
app.use(passport.session());
app.use(app.router);


app.get('/', ensureAuthenticated, site.index);
//app.get('/editor/filesystem', editor.filesystem);
//app.get('/editor/file', editor.file);
app.get('/editor', ensureAuthenticated, editor.index);

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
  console.log(req.session);
  console.log(req.isAuthenticated());
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

  var exists = path.existsSync('./repositories');
  if (!exists) {
    fs.mkdirSync('./repositories', 0777);
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
  io.sockets.on('connection', function (socket) {
    socket.emit('cwd-init', {dirname: __dirname + '/repositories'});

    socket.on('git-delete', function(data) {
      git_helper.remove_commit_push(data.file, function(err, status) {
        socket.emit('git-delete-complete', {message: ""});
      });
    });

    socket.on('commit-file', function (data) {
      //TODO...clean this up, and check for errors
      //console.log(data.file.path);

      //console.log(repository);
      git_helper.commit_push_and_save(data.file, function(err, status) {
        socket.emit('commit-file-complete', {message: "Save was successful"});
      });
    });

    socket.on('run-file', function(data) {
      exec_helper.execute_program(data.file, socket);
      //git_helper.commit_push_and_save(data.file, function(err, status) {
      //  socket.emit('commit-file-complete', {message: "Save was successful"});
      //});
    });
  });
}



function mount_dav(server) {
  var jsDAV_Tree_Filesystem = require("jsDAV/lib/DAV/tree/filesystem").jsDAV_Tree_Filesystem;
  //jsDAV.debugMode = true;
  davServer = jsDAV.mount({
    path: __dirname + "/repositories",
    mount: '/filesystem',
    plugins: ["browser", "codesearch", "tree", "filelist", "filesearch", "locks", "mount", "temporaryfilefilter"],
    server: server,
    standalone: false,
    tree: new jsDAV_Tree_Filesystem(__dirname + "/repositories")
  });
  console.log('webdav filesystem mounted');
}

