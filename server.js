var express = require('express'),
    app = express(),
    site = require('./controllers/site');

app.set('view engine', 'ejs');
app.set('views', __dirname + '/views');
app.use(express.cookieParser());
app.use(express.methodOverride());
app.use(express.static(__dirname + '/public'));

app.get('/', site.index);

app.listen(3001);
console.log('listening on port 3001');
