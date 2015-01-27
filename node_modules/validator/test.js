v = require('./');
s = '';
for (var i = 0; i < 100; i++) {
    s += '-/';
        t = Date.now();
            url = 'http://example.org/' + s + '@';
                try {
                        v.check(url).isUrl();
                            } catch (e) { }
                                console.log('i = ' + i + ", len: " + url.length + ", time: " + (Date.now() - t) + " ms");
                                }
