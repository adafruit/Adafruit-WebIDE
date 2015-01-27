/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var Assert = require('test/assert.js').Assert
exports.Assert = function StreamAssert() {
  var assert = Assert.apply(this, arguments)
  assert.equalElements = function equalElements(stream, elements, message) {
  }
}
var list = require('../core.js').list

function test(assert, expected) {
  var actual = [], isStopped = false

  list.apply(null, expected)(function next(element) {
    actual.push(element)
  }, function stop(error) {
    isStopped = true
    assert.equal(error, undefined, 'stream is stopped without an error')
  })
  assert.ok(isStopped, 'stream is stopped')
  assert.deepEqual(actual, expected, "all elements were yielded in right order")
}

exports['test empty list'] = function(assert) {
  test(assert, [])
}

exports['test number list'] = function(assert) {
  test(assert, [ 1, 2, 3 ])
}

exports['test mixed list'] = function(assert) {
  test(assert, [ 'a', 2, 'b', 4, {}, function() {}, /foo/, new Error('Boom!') ])
}


if (module == require.main)
  require('test').run(exports);

});
