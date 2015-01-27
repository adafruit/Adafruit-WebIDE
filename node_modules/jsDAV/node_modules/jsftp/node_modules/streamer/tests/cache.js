/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true setTimeout: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    cache = streamer.cache, list = streamer.list
var utils = require('./utils.js'),
    test = utils.test, pipe = utils.pipe, read = utils.read

exports['test cache an empty list'] = function(assert, done) {
  test(assert, done, cache(list()), [])
}

exports['test number list'] = function(assert, done) {
  test(assert, done, cache(list(1, 2, 3)), [ 1, 2, 3 ])
}

exports['test mixed list'] = function(assert, done) {
  var object = {}, func = function() {}, exp = /foo/, error = new Error('Boom!')
  test(assert, done, cache(list('a', 2, 'b', 4, object, func, exp, error)),
       [ 'a', 2, 'b', 4, object, func, exp, error  ])
}

exports['test read cache before source is stopped'] = function(assert) {
  var readers = [], stops = []
  var stream = cache(pipe(readers)), source = readers[0]

  // Start reading before there is anything being cached form source.
  var buf1 = read(stream, stops.push.bind(stops))
  source.next('a')
  source.next('b')

  // Start reading before source is stopped.
  var buf2 = read(stream, stops.push.bind(stops))

  source.next('c')
  source.next('d')
  source.stop('Boom!')

  // Start reading after source is stopped.
  var buf3 = read(stream, stops.push.bind(stops))

  assert.equal(readers.length, 1, 'source stream is read only once')
  assert.deepEqual(stops, [ 'Boom!', 'Boom!', 'Boom!' ],
                   'All streams were stopped with a same reason')
  assert.deepEqual(buf1, ['a', 'b', 'c', 'd' ],
                   'reading before cached includes all elements from source')
  assert.deepEqual(buf1, buf2,
                   'reading cache before and while it is cached is same')
  assert.deepEqual(buf1, buf3,
                   'reading cache before and after it is cached is same')

}

exports['test interrupt reading from cache'] = function(assert) {
  var readers = [], stops = []
  var stream = cache(pipe(readers)), source = readers[0]

  // Start reading before there is anything being cached form source.
  var buf1 = read(stream, stops.push.bind(stops))
  var buf1sub = read(stream, stops.push.bind(stops), 3)
  source.next('a')
  source.next('b')

  // Start reading before source is stopped.
  var buf2 = read(stream, stops.push.bind(stops))
  var buf2sub = read(stream, stops.push.bind(stops), 2)

  source.next('c')
  source.next('d')
  source.stop()

  // Start reading after source is stopped.
  var buf3 = read(stream, stops.push.bind(stops))
  var buf3sub = read(stream, stops.push.bind(stops), 4)

  assert.equal(readers.length, 1, 'source stream is read only once')
  assert.deepEqual(stops, [ undefined, undefined, undefined ],
                   'All streams were stopped with a same reason')
  assert.deepEqual(buf1, ['a', 'b', 'c', 'd' ],
                   'reading before cached includes all elements from source')
  assert.deepEqual(buf1, buf2,
                   'reading cache before and while it is cached is same')
  assert.deepEqual(buf1, buf3,
                   'reading cache before and after it is cached is same')

  assert.deepEqual(buf1sub, [ 'a', 'b', 'c' ],
                   'reading before cached includes all read elements')
  assert.deepEqual(buf2sub, [ 'a', 'b' ],
                   'reading during cacheing includes all read elements')
  assert.deepEqual(buf3sub, [ 'a', 'b', 'c', 'd' ],
                   'reading after cached includes all read elements')
}

if (module == require.main)
  require('test').run(exports)

});
