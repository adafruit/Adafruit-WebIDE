/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true undef: true es5: true node: true devel: true
         forin: false latedef: false */
/*global define: true */

!(typeof define !== "function" ? function($){ $(require, exports, module); } : define)(function(require, exports, module, undefined) {

'use strict';

var core = require('./core'),
    map = core.map, merge = core.merge, list = core.list, append = core.append

function normalize(source) {
  /**
  Normalizes given `source` stream, so that it's guaranteed that
  stop is called only once & no next is called after that.
  **/

  return function stream(next, stop) {
    var stopped = false, reading = true
    source(function onElement(element) {
      if (!stopped && false !== reading)
        return (reading = next(element))
    }, function onStop(reason) {
      return stopped ? nil : (stopped = true, stop(reason))
    })
  }
}
exports.normalize = normalize

function tree(isBranch, children, root) {
  /**
  Returns a lazy stream of the nodes in a tree, via a depth-first walk.
  `isBranch` must be a function that takes one argument and returns true if
  passed a node that can have children (but may not). `children` must be a
  function that takes one argument and returns a stream of the children. Will
  only be called on nodes for which `isBranch` returns true. `root` is the root
  node of the tree.
  **/

  return (function walk(node) {
    return function stream(next, stop) {
      var $ = isBranch(node)
      !(typeof($) === 'function' ? $ : list($))(function(isBranch) {
        (isBranch ?
         append(list(node), merge(map(walk, children(node)))) :
         list(node))(next, stop)
      })
    }
  })(root)
}
exports.tree = tree

});
