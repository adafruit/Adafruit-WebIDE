var exec = require("child_process").exec;

exec('ruby -e "puts \'Hello from Ruby!\'"', function (err, stdout, stderr) {
    console.log(stdout);
});