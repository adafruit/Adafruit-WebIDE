/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true setTimeout: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    append = streamer.append, list = streamer.list
var test = require('./utils.js').test

exports['test append empty streams'] = function(assert, done) {
  test(assert, done, append(list(), list()), [])
}

exports['test append empty'] = function(assert, done) {
  test(assert, done, append(list(1, 2), list()), [1, 2])
}

exports['test append to empty'] = function(assert, done) {
  test(assert, done, append(list(), list(3, 4)), [3, 4])
}

exports['test append many streams'] = function(assert, done) {
  var stream = append(list(1, 2), list(), list('a', 'b'), list())
  test(assert, done, stream, [1, 2, 'a', 'b'])
}

exports['test append sync & async streams'] = function(assert, done) {
  function async(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop()
      if (false !== next(x--)) setTimeout(onTimeout, 0)
    }, 0)
  }

  var stream = append(async, list(), async, list('a', 'b'))
  test(assert, done, stream, [ 3, 2, 1, 3, 2, 1, 'a', 'b' ])
}

exports['test append & reappend'] = function(assert, done) {
    function async(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop()
      if (false !== next(x--)) setTimeout(onTimeout, 0)
    }, 0)
  }

  var stream = append(async, list('a', 'b'))
  stream = append(stream, list('||'), stream)
  test(assert, done, stream, [ 3, 2, 1, 'a', 'b', '||', 3, 2, 1, 'a', 'b' ])
}

exports['test map broken stream'] = function(assert, done) {
  var buffer = []
  function async(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error("Boom!"))
      if (false !== next(x--)) setTimeout(onTimeout, 0)
    }, 0)
  }
  
  var stream = append(list('>'), async, list(1, 2), async)
  stream(function next(x) { buffer.push(x) }, function stop(error) {
    assert.equal(error.message, 'Boom!', 'error propagated')
    assert.deepEqual(buffer, [ '>', 3, 2, 1 ],
                     'all values were yielded before error')
    done()
  })
}

exports['test interrupt apended stream'] = function(assert) {
  var letters = list('a', 'b', 'c', 'd')
  var numbers = list(1, 2, 3, 3, 5)
  var stream = append(letters, numbers)
  var buffer = []
  var stopped = []
  stream(function onElement(element) {
    buffer.push(element)
    if (buffer.length === 3) return false
  }, stopped.push.bind(stopped))
  assert.deepEqual(buffer, [ 'a', 'b', 'c' ],
                   'stream yielded elements until it was interrupted')

  buffer = []
  stream(function onElement(element) {
    buffer.push(element)
    if (buffer.length === 4) return false
  }, stopped.push.bind(stopped))
  assert.deepEqual(buffer, [ 'a', 'b', 'c', 'd' ],
                   'stream yielded elements until it was interrupted')

  buffer = []
  stream(function onElement(element) {
    buffer.push(element)
    if (buffer.length === 7) return false
  }, stopped.push.bind(stopped))
  assert.deepEqual(buffer, [ 'a', 'b', 'c', 'd', 1, 2, 3 ],
                   'stream yielded elements until it was interrupted')

  assert.equal(stopped.length, 0, 'interrupted streams do not stop')
}


if (module == require.main)
  require('test').run(exports);

});
