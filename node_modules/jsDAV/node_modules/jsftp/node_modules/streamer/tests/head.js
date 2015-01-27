/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true setTimeout: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    head = streamer.head, list = streamer.list
var test = require('./utils.js').test

exports['test head empty'] = function(assert, done) {
  var empty = list()
  test(assert, done, head(empty), [])
}

exports['test head default to 1'] = function(assert, done) {
  var numbers = list(1, 2, 3, 4)
  test(assert, done, head(numbers), [1])
}

exports['test head smaller stream'] = function(assert, done) {
  var numbers = list(1, 2, 3)
  test(assert, done, head(numbers, 5), [1, 2, 3])
}

exports['test head of async stream'] = function(assert, done) {
  function stream(next, stop) {
    var x = 5
    setTimeout(function onTimeout() {
      if (!x) return stop()
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  test(assert, done, head(stream, 3), [ 5, 4, 3 ])
}

exports['test head before stream error'] = function(assert, done) {
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  test(assert, done, head(stream, 3), [3, 2, 1])
}

exports['test head on broken stream'] = function(assert, done) {
  var buffer = []
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  head(stream, 5)(function next(x) { buffer.push(x) }, function stop(error) {
    assert.equal(error.message, 'Boom!', 'error propagated to mapped stream')
    assert.deepEqual(buffer, [3, 2, 1], 'all values were yielded before error')
    done()
  })
}

if (module == require.main)
  require('test').run(exports)

});
