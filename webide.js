var forever = require('forever-monitor');

var child = new (forever.Monitor)('server.js', {
  silent: false,
  killTree: true,
  minUptime: 5000,
  watch: true,
  sourceDir: __dirname + '/editor'
  //watchIgnoreDotFiles: '.foreverignore',
  //'logFile': 'logs/forever.log',
  //'outFile': 'logs/stdout.log',
  //'errFile': 'logs/stderr.log'
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