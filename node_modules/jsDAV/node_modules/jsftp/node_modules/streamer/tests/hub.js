/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var hub = require('../core.js').hub
var utils = require('./utils.js'),
    test = utils.test, pipe = utils.pipe

function copy(source) {
  var buffer = [], reason;
  source(function onElement(element) {
    buffer.push(element)
  }, function onStop(error) {
    reason = error
  })
  return function stream(next, stop) {
    var index = 0, length = buffer.length
    while (index < length) next(buffer[index++])
    if (stop) stop(reason)
  }
}

exports['test hub with normal stop'] = function(assert, done) {
  var readers = [], copies = []
  var source = hub(pipe(readers)), stream = readers[0]

  stream.next(1)
  copies.push(copy(source))
  stream.next('a')
  stream.next('b')
  copies.push(copy(source))
  stream.next(2)
  stream.next('last')
  stream.stop()
  copies.push(copy(source))

  assert.equal(readers.length, 1, "stream was read only once")
  test(assert, Object, copies[0], [ 'a', 'b', 2, 'last'])
  test(assert, Object, copies[1], [ 2, 'last' ])
  test(assert, done, copies[2], [])
}

exports['test hub with error stop'] = function(assert, done) {
  var readers = [], copies = [], error = new Error('boom')
  var source = hub(pipe(readers)), stream = readers[0]

  stream.next(1)
  copies.push(copy(source))
  stream.next('a')
  stream.next('b')
  copies.push(copy(source))
  stream.next(2)
  stream.next('last')
  stream.stop(error)
  copies.push(copy(source))

  assert.equal(readers.length, 1, "stream was read only once")
  test(assert, Object, copies[0], [ 'a', 'b', 2, 'last'], error)
  test(assert, Object, copies[1], [ 2, 'last' ], error)
  test(assert, done, copies[2], [], error)
}

exports['test hub with interrupt'] = function (assert) {
  var readers = [], buffers = [], stops = []
  var source = hub(pipe(readers)), stream = readers[0]

  stream.next('a')
  stream.next('b')

  buffers[0] = []
  source(function (element) {
    buffers[0].push(element)
    if (buffers[0].length === 3) return false
  }, stops.push.bind(stops))

  stream.next('c')
  stream.next('d')

  buffers[1] = []
  source(function (element) {
    buffers[1].push(element)
    if (buffers[1].length === 4) return false
  }, stops.push.bind(stops))

  stream.next('e')

  buffers[2] = []
  source(function (element) {
    buffers[2].push(element)
    if (buffers[2].length === 4) return false
  }, stops.push.bind(stops))



  stream.next('f')
  stream.next('g')
  stream.next('h')
  stream.stop()

  assert.deepEqual(buffers[0], [ 'c', 'd', 'e' ],
                   'interrupted after 3 elements')
  assert.deepEqual(buffers[1], [ 'e', 'f', 'g', 'h' ],
                   'interrupted after 4 elemens')
  assert.deepEqual(buffers[2], [ 'f', 'g', 'h' ],
                   'read all remaining elements')
  assert.deepEqual(stops, [undefined], 'one stream reader stopped')

}

if (module == require.main)
  require('test').run(exports)

});
