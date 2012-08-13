var express = require('express'),
    app = express(),
    passport = require('passport'),
    util = require('util'),
    BitbucketStrategy = require('passport-bitbucket').Strategy,
    site = require('./controllers/site'),
    editor = require('./controllers/editor'),
    user = require('./controllers/user');

//TODO
//https://github.com/mikeal/request (OAuth Signing)

var BITBUCKET_CONSUMER_KEY = "c7XXD9UtX3DWMF3Aa4";
var BITBUCKET_CONSUMER_SECRET = "UpFuYfDcWEGdR9KW5gbe5gatbhnGDSSp";

// Passport session setup.
//   To support persistent login sessions, Passport needs to be able to
//   serialize users into and deserialize users out of the session.  Typically,
//   this will be as simple as storing the user ID when serializing, and finding
//   the user by ID when deserializing.  However, since this example does not
//   have a database of user records, the complete Bitbucket profile is
//   serialized and deserialized.
passport.serializeUser(function(user, done) {
  done(null, user);
});

passport.deserializeUser(function(obj, done) {
  done(null, obj);
});

// Use the BitbucketStrategy within Passport.
//   Strategies in passport require a `verify` function, which accept
//   credentials (in this case, a token, tokenSecret, and Bitbucket profile),
//   and invoke a callback with a user object.
passport.use(new BitbucketStrategy({
    consumerKey: BITBUCKET_CONSUMER_KEY,
    consumerSecret: BITBUCKET_CONSUMER_SECRET,
    callbackURL: "http://127.0.0.1:3001/auth/bitbucket/callback"
  },
  function(token, tokenSecret, profile, done) {
    // asynchronous verification, for effect...
    process.nextTick(function () {
      
      // To keep the example simple, the user's Bitbucket profile is returned to
      // represent the logged-in user.  In a typical application, you would want
      // to associate the Bitbucket account with a user record in your database,
      // and return that user instead.
      profile.token = token;
      profile.token_secret = tokenSecret;
      profile.consumer_key = BITBUCKET_CONSUMER_KEY;
      profile.consumer_secret = BITBUCKET_CONSUMER_SECRET;
      return done(null, profile);
    });
  }
));

app.set('view engine', 'jade');
app.set('views', __dirname + '/views');
app.use(express.logger());
app.use(express.cookieParser('keyboard cat'));
app.use(express.bodyParser());
app.use(express.methodOverride());
app.use(express.session());
app.use(passport.initialize());
app.use(passport.session());
app.use(app.router);
app.use(express.static(__dirname + '/public'));

app.get('/', ensureAuthenticated, site.index);
app.get('/editor/filesystem', editor.filesystem);
app.get('/editor/:repository', editor.index);

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
//   login page.  Otherwise, the primary route function function will be called,
//   which, in this example, will redirect the user to the home page.
app.get('/auth/bitbucket/callback',
  passport.authenticate('bitbucket', { failureRedirect: '/login' }),
  function(req, res) {
    res.redirect('/');
  });

app.listen(3001);
console.log('listening on port 3001');

// Simple route middleware to ensure user is authenticated.
//   Use this route middleware on any resource that needs to be protected.  If
//   the request is authenticated (typically via a persistent login session),
//   the request will proceed.  Otherwise, the user will be redirected to the
//   login page.
function ensureAuthenticated(req, res, next) {
  if (req.isAuthenticated()) { return next(); }
  res.redirect('/login');
}
