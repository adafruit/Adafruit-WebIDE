/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true undef: true es5: true node: true devel: true
         forin: true latedef: false supernew: true */
/*global define: true */

!(typeof(define) !== "function" ? function($) { $(typeof(require) !== 'function' ? (function() { throw Error('require unsupported'); }) : require, typeof(exports) === 'undefined' ? this : exports, typeof(module) === 'undefined' ? {} : module); } : define)(function(require, exports, module) {

"use strict";

var test = require('./utils.js').test
var tree = require('../experimental').tree
var streamer = require('../core'),
    map = streamer.map, list = streamer.list

exports['test on json'] = function(assert, done) {
  var source = { a1: { a2: { a3: 3 } }, b1: 1, c1: { c2: 2 } }

  var output = tree(function isBranch($) {
    return typeof($) === 'object'
  }, function children($) {
    return list.apply(null, Object.keys($).map(function($2) { return $[$2] }))
  }, source)

  test(assert, done, output, [
    source,
    source.a1,
    source.a1.a2,
    source.a1.a2.a3,
    source.b1,
    source.c1,
    source.c1.c2
  ])
}


if (module == require.main)
  require('test').run(exports)

});
