/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true setTimeout: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    zip = streamer.zip, list = streamer.list
var test = require('./utils.js').test

exports['test zip with empty'] = function(assert, done) {
  var empty = list()
  var numbers = list(1, 2, 3)
  var zipped = zip(empty, numbers)

  test(assert, done, zipped, [])
}

exports['test zip 2 lists'] = function(assert, done) {
  var numbers = list(1, 2, 3, 4)
  var letters = list('a', 'b', 'c', 'd')
  var zipped = zip(numbers, letters)
  test(assert, done, zipped, [ [ 1, 'a' ], [ 2, 'b' ], [ 3, 'c' ], [ 4, 'd' ] ])
}

exports['test zip sync stream with async stream'] = function(assert, done) {
  function a(next, stop) {
    var x = 5
    setTimeout(function onTimeout() {
      if (!x) return stop()
      if (false !== next(x--)) setTimeout(onTimeout, 0)
    }, 0)
  }
  var b = list('a', 'b', 'c', 'd', 'e')
  var c = list('~', '@', '!', '#')

  var zipped = zip(a, b, c)

  test(assert, done, zipped, [
    [ 5, 'a', '~'  ],
    [ 4, 'b', '@' ],
    [ 3, 'c', '!' ],
    [ 2, 'd', '#' ]
  ])
}

exports['test zip with late error'] = function(assert, done) {
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      if (false !== next(x--)) setTimeout(onTimeout, 0)
    }, 0)
  }
  var letters = list('a', 'b', 'c')
  var zipped = zip(letters, stream)

  test(assert, done, zipped, [
    [ 'a', 3 ],
    [ 'b', 2 ],
    [ 'c', 1 ]
  ])
}

exports['test zip with early error'] = function(assert, done) {
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      if (false !== next(x--)) setTimeout(onTimeout, 0)
    }, 0)
  }
  var letters = list('a', 'b', 'c', 'd')
  var zipped = zip(stream, letters)
  var buffer = []
  zipped(function onTuple(tuple) {
    buffer.push(tuple)
  }, function onStop(error) {
    assert.deepEqual(buffer, [
      [ 3, 'a' ],
      [ 2, 'b' ],
      [ 1, 'c' ]
    ], 'Stream yielded all tuples in right order before error in source stream')
    assert.equal(error.message, 'Boom!', 'Stream is stopped with error')
    done()
  })
}

exports['test interrupt zipped stream'] = function(assert) {
  var letters = list('a', 'b', 'c', 'd')
  var numbers = list(1, 2, 3, 3, 5)
  var zipped = zip(numbers, letters)
  var buffer = []
  var stopped = []
  zipped(function onTuple(tuple) {
    buffer.push(tuple)
    if (buffer.length === 4) return false
  }, function onStop(error) {
    stopped.push(error)
  })
  assert.equal(stopped.length, 0, 'interrupted streams do not stop')
  assert.deepEqual(buffer, [ [1, 'a'], [ 2, 'b' ], [ 3, 'c' ], [ 3, 'd' ] ],
                   'stream yielded elements until it was interrupted')
}

if (module == require.main)
  require('test').run(exports)

});
