var express = require('express'),
    app = express(),
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
    nStore = require('nstore'),
    nStoreSession = require('nStoreSession/lib/nstore-session');

var davServer;
console.log(__dirname);

var BITBUCKET_CONSUMER_KEY = "c7XXD9UtX3DWMF3Aa4";
var BITBUCKET_CONSUMER_SECRET = "UpFuYfDcWEGdR9KW5gbe5gatbhnGDSSp";

var ADAFRUIT_REPOSITORY = "Adafruit-Raspberry-Pi-Python-Code";
var ADAFRUIT_REPOSITORY_REMOTE = 'git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git';


// Passport session setup.
//   To support persistent login sessions, Passport needs to be able to
//   serialize users into and deserialize users out of the session.  Typically,
//   this will be as simple as storing the user ID when serializing, and finding
//   the user by ID when deserializing.  However, since this example does not
//   have a database of user records, the complete Bitbucket profile is
//   serialized and deserialized.
passport.serializeUser(function(user, done) {
  var users = nStore.new(__dirname + '/users.db', function () {
    users.save(user.username, user, function(err) {
      done(null, user.username);
    });
  });
});

passport.deserializeUser(function(obj, done) {
  var users = nStore.new(__dirname + '/users.db', function () {
    users.get(obj, function (err, doc, key) {
        //if (err) { throw err; }
        done(null, doc);
    });
  });
});

// Use the BitbucketStrategy within Passport.
//   Strategies in passport require a `verify` function, which accept
//   credentials (in this case, a token, tokenSecret, and Bitbucket profile),
//   and invoke a callback with a user object.
passport.use(new BitbucketStrategy({
    consumerKey: BITBUCKET_CONSUMER_KEY,
    consumerSecret: BITBUCKET_CONSUMER_SECRET,
    callbackURL: "http://raspberrypi.local:3000/auth/bitbucket/callback"
    //callbackURL: "http://76.17.224.82:3000/auth/bitbucket/callback"
  },
  function(token, tokenSecret, profile, done) {
    // asynchronous verification, for effect...
    process.nextTick(function () {
      profile.token = token;
      profile.token_secret = tokenSecret;
      profile.consumer_key = BITBUCKET_CONSUMER_KEY;
      profile.consumer_secret = BITBUCKET_CONSUMER_SECRET;

      //TODO REFACTOR THIS MESS
      request_helper.list_repositories(profile, function(err, list) {
        var exists = list.some(function(repository) {
          return (repository.name === ADAFRUIT_REPOSITORY);
        });
        git_helper.clone_adafruit_libraries(ADAFRUIT_REPOSITORY, ADAFRUIT_REPOSITORY_REMOTE, function() {
          if (!exists) {
            request_helper.create_repository(profile, ADAFRUIT_REPOSITORY, function(err, response) {
              console.log("created adafruit repository in bitbucket");
              git_helper.update_remote(profile, ADAFRUIT_REPOSITORY, function(err, response) {
                console.log("updated remote for adafruit repository");
                return done(null, profile);
              });

            });
          } else {
            git_helper.update_remote(profile, ADAFRUIT_REPOSITORY, function(err, response) {
              return done(null, profile);
            });
          }
        });
      });

    });
  }
));

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
//session & cookie
//var sessionStore = new express.session.MemoryStore({reapInterval: 60000 * 10});
app.use(express.cookieParser());
app.use(express.session({
  store: new nStoreSession(),
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

app.post('/create/repository', editor.create_repository);

app.get('/login', user.login);
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
  if (req.isAuthenticated()) { return next(); }
  res.redirect('/login');
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
  return server.listen(3000);
}

function socket_listeners() {
  io.sockets.on('connection', function (socket) {
    socket.emit('news', { hello: 'world' });
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

