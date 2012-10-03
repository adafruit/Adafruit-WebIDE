var forever = require('forever-monitor'),
    path = require('path'),
    fs = require('fs');

//check for the existence of the logs directory, if it doesn't
//exist, create it prior to starting the child process.
var exists = path.existsSync(__dirname + '/logs');
if (!exists) {
  fs.mkdirSync(__dirname + '/logs', 0755);
  console.log('created repositories folder');
}

/*
 * This is the definition of the child process, which
 * is the actual editor server that forever will startup
 * and monitor.
 */
var child = new (forever.Monitor)('server.js', {
  silent: false,
  killTree: true,
  minUptime: 5000,
  watch: true,
  sourceDir: __dirname + '/editor',
  'logFile': __dirname + '/logs/forever.log',
  'outFile': __dirname + '/logs/stdout.log',
  'errFile': __dirname + '/logs/stderr.log'
});

child.on('exit', function () {
  console.log('server exited');
});

child.on('watch:restart', function (details) {
  console.log(details);
});

child.on('error', function (err) {
  console.log(err);
});

console.log("Starting the Adafruit WebIDE...");
child.start();