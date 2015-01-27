/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true setTimeout: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    tail = streamer.tail, list = streamer.list
var test = require('./utils.js').test

exports['test tail empty'] = function(assert, done) {
  var empty = list()
  test(assert, done, tail(empty), [])
}

exports['test tail default to 1'] = function(assert, done) {
  var numbers = list(1, 2, 3, 4)
  test(assert, done, tail(numbers), [2, 3, 4])
}

exports['test tail smaller stream'] = function(assert, done) {
  var numbers = list(1, 2, 3)
  test(assert, done, tail(numbers, 5), [])
}

exports['test tail of async stream'] = function(assert, done) {
  function stream(next, stop) {
    var x = 5
    setTimeout(function onTimeout() {
      if (!x) return stop()
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  test(assert, done, tail(stream), [ 4, 3, 2, 1 ])
}

exports['test stream error in tail'] = function(assert, done) {
  var buffer = []
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  tail(stream)(function next(x) { buffer.push(x) }, function stop(error) {
    assert.equal(error.message, 'Boom!', 'error propagated to tail')
    assert.deepEqual(buffer, [2, 1], 'all values yielded in order before error')
    done()
  })
}

exports['test stream error before tail'] = function(assert, done) {
  var buffer = []
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  tail(stream, 5)(function next(x) { buffer.push(x) }, function stop(error) {
    assert.equal(error.message, 'Boom!', 'error propagated to mapped stream')
    assert.deepEqual(buffer, [], 'no values yielded before error')
    done()
  })
}

if (module == require.main)
  require('test').run(exports)

});
