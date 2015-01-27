/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true setTimeout: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    reduce = streamer.reduce, list = streamer.list
var test = require('./utils.js').test

exports['test reduce on empty'] = function(assert, done) {
  var empty = list()
  var reduced = reduce(function onEach(element) {
    assert.fail('reducer was called on empty list')
  }, empty, null)
  test(assert, done, reduced, [ null ])
}

exports['test reduce to sum'] = function(assert, done) {
  var numbers = list(1, 2, 3, 4)
  var sum = reduce(function onElement(previous, current) {
    return (previous || 0) + current
  }, numbers)
  test(assert, done, sum, [ 10 ])
}

exports['test reduce async stream'] = function(assert, done) {
  function stream(next, stop) {
    var x = 5
    setTimeout(function onTimeout() {
      if (!x) return stop()
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  var sum = reduce(function(previous, current) {
    return previous + current
  }, stream, 3)
  test(assert, done, sum, [ 18 ])
}

exports['test reduce broken stream'] = function(assert, done) {
  function stream(next, stop) {
    var x = 3
    setTimeout(function onTimeout() {
      if (!x) return stop(new Error('Boom!'))
      next(x--)
      setTimeout(onTimeout, 0)
    }, 0)
  }
  var sum = reduce(function(x, y) { return x + y }, stream)
  sum(function next(x) {
    assert.fail('should not yield value if stream failed')
  }, function stop(error) {
    assert.equal(error.message, 'Boom!', 'error propagated to reduced stream')
    done()
  })
}

if (module == require.main)
  require('test').run(exports)

});
