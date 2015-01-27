/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

var streamer = require('../core.js'),
    stack = streamer.stack, list = streamer.list, head = streamer.head
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

exports['test stack with normal stop'] = function(assert, done) {
  var readers = [], copies = []
  var stream, source = stack(pipe(readers))

  copies.push(copy(source))
  stream = readers[0]
  stream.next(1)
  stream.next('a')
  stream.next('b')
  copies.push(copy(source))
  stream.next(2)
  stream.next('last')
  stream.stop()
  copies.push(copy(source))

  assert.equal(readers.length, 1, "stream was read only once")
  test(assert, Object, copies[0], [ 1, 'a', 'b', 2, 'last'])
  test(assert, Object, copies[1], [])
  test(assert, done, copies[2], [])
}

exports['test stack with error stop'] = function(assert, done) {
  var readers = [], copies = [], error = new Error('boom')
  var stream, source = stack(pipe(readers))

  copies.push(copy(source))
  stream = readers[0]

  stream.next(1)
  stream.next('a')
  stream.next('b')
  copies.push(copy(source))
  stream.next(2)
  stream.next('last')
  stream.stop(error)
  copies.push(copy(source))

  assert.equal(readers.length, 1, "stream was read only once")
  test(assert, Object, copies[0], [ 1, 'a', 'b', 2, 'last'], error)
  test(assert, Object, copies[1], [], error)
  test(assert, done, copies[2], [], error)
}

exports['test stack with interrupt'] = function (assert) {
  var readers = [], buffers = [], stops = []
  var stream, source = stack(pipe(readers))

  buffers[0] = []
  source(function (element) {
    buffers[0].push(element)
    if (buffers[0].length === 3) return false
  }, stops.push.bind(stops))
  stream = readers[0]

  buffers[1] = []
  source(function (element) {
    buffers[1].push(element)
    if (buffers[1].length === 4) return false
  }, stops.push.bind(stops))

  stream.next('a')
  stream.next('b')
  stream.next('c')
  stream.next('d')
  stream.next('e')
  stream.next('f')
  stream.next('g')

  buffers[2] = []
  source(function (element) {
    buffers[2].push(element)
    if (buffers[2].length === 4) return false
  }, stops.push.bind(stops))


  stream.next('h')
  stream.stop()

  assert.deepEqual(buffers[0], [ 'a', 'b', 'c' ],
                   'interrupted after 3 elements')
  assert.deepEqual(buffers[1], [ 'd', 'e', 'f', 'g' ],
                   'interrupted after 4 elemens')
  assert.deepEqual(buffers[2], [ 'h' ],
                   'read all remaining elements')
  assert.deepEqual(stops, [undefined], 'one stream reader stopped')
}

exports['test pull form stack on read'] = function(assert, done) {
  function async(next, stop) {
    setTimeout(function onTimeout(x) {
      if (!x) return stop()
      if (false !== next(x)) setTimeout(onTimeout, 0, --x)
    }, 0, 3)
  }

  var source = stack(list(async, list('|'), list('a', 'b'), list())),
      buffer = [], stops = [], nil

  !function read(callback) {
    var open = 0
    head(source)(function(stream) {
      var elements = []
      open += 2
      buffer.push(elements)
      stream(function(element) {
        elements.push(element)
      }, function onStop(error) {
        open --
        stops.push(error)
        if (!open) read(callback)
      })
    }, function onStop(error) {
      open --
      stops.push(error)
      if (!open) read(callback)
      else if (open === -1) callback()
    })
  }(function() {
    assert.deepEqual(buffer, [
      [ 3, 2, 1 ],
      ['|'],
      [ 'a', 'b' ],
      []
    ], 'all elements were yielded')

    assert.deepEqual(stops, [
      nil, nil,
      nil, nil,
      nil, nil,
      nil, nil,
      nil
    ], 'stopped as expected')
    done()
  })
}

if (module == require.main)
  require('test').run(exports)

});
