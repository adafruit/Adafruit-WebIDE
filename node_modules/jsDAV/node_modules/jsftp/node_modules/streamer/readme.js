/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true undef: true es5: true node: true devel: true
         forin: true latedef: false globalstrict: true */
/*global define: true setInterval: true */

/* html version: http://jeditoolkit.com/streamer/docs/readme.html */

'use strict';

// In computing, the term stream is used in a number of ways, in all cases
// referring to a sequence of elements made available over time.


// Let's create a very basic stream representing a sequence of elements
// from 1 to 3.

function stream(next) {
  [ 1, 2, 3 ].forEach(function(element) {
    next(element)
  })
}

// From this example we can define a stream as:
// a function representing a sequence of elements. It can be read by calling
// with one function argument, which will be called back with each element of
// the sequence.

// So we can print our stream like this:
stream(function onEach(element) {
  console.log(element)
})

//      1
//      2
//      3

// Or, we can create a convenience
// [high-order function](http://en.wikipedia.org/wiki/Higher-order_function)
// for printing streams.

function print(stream) {
  stream(function onEach(element) {
    console.log(element)            // Print each element of the sequence.
  })
}

// And, print stream with it:
print(stream)

//      1
//      2
//      3

// Good, but!
// Stream is a sequence of elements **made available over time**.
// In other words a sequence may be lazy, and our stream definition needs
// refinement:
//
// Stream is a function representing a sequence of elements. It MAY be read by
// calling it with one function argument, that will be called back with each
// element when it becomes available.

// Let's create a function `numbers`, that takes `min` and
// `max` numbers and returns a lazy stream of random numbers in a given range.
// To make the stream lazy, we will make its new elements available every 20ms.
function numbers(min, max) { // Another high-order function to make streams
  var delta = max - min
  return function stream(next) { // Actual stream that generates 
    setInterval(function generate() {
      // We yield a random number in given range every 20ms.
      next(min + Math.round(Math.random() * delta))
    }, 20)
  }
}

// Make a stream of random numbers in the 0 - 100 range.
var numberStream = numbers(0, 100)
// And we print it!!
print(numberStream)

//      29
//      33
//      45
//      ....

// Oops! The stream keeps printing these numbers infinitely. Right, that's because
// a stream is infinite! So we may have finite and infinite streams, the difference
// being that finite streams end / stop at some point. And if a stream stops we need
// to know when that happens. To do that we will add a second, optional `stop`
// callback argument that MUST be called once stream reaches its end. Let's
// redefine our `print` function with this in mind:

function print(stream) {
  console.log(">>>")                    // Opening stream for reading
  stream(function onElement(element) {
    console.log(element)                // Print each element of stream.
  }, function onStop() {
    console.log("<<<")                  // Stream is stopped.
  })
}

// Now we need a stream to print. Instead of creating another basic stream,
// this time we will take more generic approach by defining a function that
// takes an array as an argument and returns a stream of its elements:

function list(array) {
  return function stream(next, stop) {
    array.forEach(function(element) {
      next(element)
    })
    stop()
  }
}

// Great let's print something now!

print(list(1, 2, 3))

// Right, we should have passed an array to the list. Yeah, so shit happens! And
// when it happens to the stream, it needs to do something about it. The only
// reasonable thing is to recover, and if that is not possible then stop and
// report the reason of failure. This means that the `stop` callback MAY be called
// with an error argument, indicating a reason of failure!

// Let's adjust our print and streams to do that:

function print(stream) {
  console.log(">>>")                      // Opening stream for reading
  stream(function onElement(element) {
    console.log(element)                  // Print each element of stream.
  }, function onStop(error) {
    if (!error) return console.log('<<<') // If no error is passed, stream ended
    // If there is an error print it out as well.
    console.log('!!!')
    console.error(error)
  })
}

// Let's make another version of a function that returns a stream of given
// elements, in this case though we will use arguments instead of requiring
// an array argument.
function list() {
  // Capture arguments as an array.
  var elements = Array.prototype.slice.call(arguments, 0)
  // Stream takes two callback arguments, the first is called with each element
  // when it becomes available, and the second after calling first with all the
  // elements of the stream.
  return function stream(next, stop) {
    // Yield each element of the stream by calling the `next`. callback.
    elements.forEach(function(element) {
      next(element)
    })
    // When we reach the end we stop a stream by calling the `stop` callback
    // if it's passed.
    if (stop) stop()
  }
}

// Another attempt to print:
print(list(1, 2, 3))

//      >>>
//      1
//      2
//      3
//      <<<

// Let's refine our stream definition again:

// _Stream is a function representing a sequence of elements. It MAY be read by
// calling it with one function argument, that will be called every time an element
// becomes available. Stream takes a second optional function argument which
// is called once the stream is stopped, either without arguments when stream runs
// out of elements or with an error indicating the failure reason indicating why
// stream was stopped._

// Let's do something interesting from real life, like create a stream of all
// directory entries including entries from all nested directories (lstree).
//
// First we will have to create a few stream based wrappers around node's fs
// functions. We will start with a function that takes the path for a directory
// and returns a lazy stream of its entries. If reading a directory fails we
// will stop the stream with an error:

var fs = require("fs")
function ls(path) {
  return function stream(next, stop) {
    //see: [http://nodejs.org/docs/v0.4.8/api/fs.html#fs.readdir](http://nodejs.org/docs/v0.4.8/api/fs.html#fs.readdir)
    fs.readdir(path, function onEntries(error, entries) {
      var entry
      // On error we stop a stream with that error.
      if (error) return stop(error)
      // Otherwise we yield each entry.
      while ((entry = entries.shift())) next(entry)
      // Once we yielded all entries we stop a stream.
      stop()
    })
  }
}

// Try it out for the current working directory:
print(ls('./'))

//      >>>
//      .gitignore
//      History.md
//      package.json
//      readme.js
//      Readme.md
//      streamer.js
//      tests
//      <<<

// The next wrapper we will need is `fs.stat`. We define a function that `takes`
// a path and returns a lazy stream with only an element representing `stat` of
// the given `path`. A lazy steam with one element can been seen as a promise
// or deferred, but don't worry if you are not familiar with that pattern.
function stat(path) {
  return function stream(next, stop) {
    //see: [http://nodejs.org/docs/v0.4.8/api/fs.html#fs.stat](http://nodejs.org/docs/v0.4.8/api/fs.html#fs.stat)
    fs.stat(path, function onStat(error, stats) {
      // On error we stop the stream with that error.
      if (error) return stop(error)
      // We add the path to the stat itself as it will be very convenient.
      stats.path = path
      // We yield `stats` and stop the stream.
      next(stats)
      stop()
    })
  }
}

// Try it out for the current working directory:
print(stat('./'))

//      >>>
//      { dev: 234881026,
//      ino: 19933437,
//      mode: 16877,
//      nlink: 17,
//      uid: 502,
//      gid: 20,
//      rdev: 0,
//      size: 578,
//      blksize: 4096,
//      blocks: 0,
//      atime: Thu, 09 Jun 2011 10:51:25 GMT,
//      mtime: Thu, 09 Jun 2011 12:48:32 GMT,
//      ctime: Thu, 09 Jun 2011 12:48:32 GMT,
//      path: './' }
//      <<<

// Great we are done with the wrappers. Now we can list entries of the directory,
// but in order to list nested entries we need to distinguish directories
// from files. To do that we will create a function that takes a directory entries
// stream and returns a filtered stream containing only entries that are
// directories. We already can get stats from paths, so we just need to map entry
// paths to stats. Let's make a generic map function that takes a stream and a
// mapper function and returns a stream of mapped elements.

function map(lambda, source) {
  return function stream(next, stop) {
    source(function onElement(element) {
      next(lambda(element))
    }, stop)
  }
}

// Let's try to map numbers into doubled values:
print(map(function(x) { return x * 2 }, list(1, 2, 3)))

//      >>>
//      2
//      4
//      6
//      <<<

// Now we can implement a function that is the equivalent of `ls` with the
// difference that it returns a stream of paths instead of entry filenames.

var join = require("path").join
function paths(path) { return map(join.bind(null, path), ls(path)) }

// Test drive:
print(paths(process.cwd()))

//      >>>
//      /Users/gozala/Projects/streamer/History.md
//      /Users/gozala/Projects/streamer/package.json
//      ...
//      <<<

// Now we need another equivalent of `paths` that returns a stream of directory
// paths only. To do that we need to filter out directories. So let's implement
// a generic filter function that takes a stream of elements and a filter function
// and returns the steam of elements for which the filterer returned true.
function filter(lambda, source) {
  return function stream(next, stop) {
    source(function onElement(element) {
      if (lambda(element)) next(element)
    }, stop)
  }
}
// Simple example for filtering out odd numbers from a number stream.
print(filter(function(x) { return x % 2 }, list(1, 2, 3, 4)))

//      >>>
//      1
//      3
//      <<<

// Awesome, going back to our problem, to figure out weather we have a file
// path or directory path we need to map paths to stats and then filter out
// only ones from there that are directories:
function dirs(paths) { 
  var stats = map(stat, paths)
  var dirStats = filter(function(stat) { return stat.isDirectory() }, stats)
  return map(function(stat) { return stat.path }, dirStats)
}

// Unfortunately dirs not going to work, because the `stats` stream is not
// a stream of `stat` elements, it is a stream of streams that are streams of
// `stat` elements. So what we need is sort of a flattened version of that stream.
// This is easy to do with another core `merge` function:
function merge(source) {
  return function stream(next, stop) {
    var open = 1
    function onStop(error) {
      if (!open) return false
      if (error) open = 0
      else open --

      if (!open) stop(error)
    }
    source(function onStream(stream) {
      open ++
      stream(function onNext(value) { if (open) next(value) }, onStop)
    }, onStop)
  }
}

// Let's try a simple example:
print(merge(list(list(1, 2), list('a', 'b'))))

//      >>>
//      1
//      2
//      a
//      b
//      <<<

// Now we can refine our dirs function:
function dirs(paths) {
  var stats = merge(map(paths, stat))
  var dirStats = filter(function(stat) { return stat.isDirectory() }, stats)
  return map(function(stat) { return stat.path }, dirStats)
}

// Test drive:
print(dirs(paths(process.cwd())))

//      >>>
//      /Users/gozala/Projects/streamer/.git
//      /Users/gozala/Projects/streamer/node_modules
//      ...
//      <<<
//

// Finally we have all we need to implement `lstree`:
function lstree(path) {
  var entries = paths(path)
  var nested = merge(map(lstree, dirs(entries)))
  return merge(list(entries, nested))
}

// Crossing our fingers!!
print(lstree('./'))
//
//      >>>
//      .git
//      .git/COMMIT_EDITMSG
//      .git/config
//      ....
//      <<<
//

// So let's take a look back now, if we ignore all the core stream functions
// that are part of the [streamer library](https://github.com/Gozala/streamer) and
// some node `fs` wrappers, we have written code that deals with recursive
// asynchronous code, but that has a very linear flow. Take another
// look at it with all the noise removed:

function paths(path) { return map(join.bind(null, path), ls(path)) }
function dirs(paths) { 
  var stats = map(stat, paths)
  var dirStats = filter(function(stat) { return stat.isDirectory() }, stats)
  return map(function(stat) { return stat.path }, dirStats)
}
function lstree(path) {
  var entries = paths(path)
  var nested = merge(map(lstree, dirs(entries)))
  return merge(list(entries, nested))
}

// Feel free to take a look at another example of using [streams in browser]
// (http://jeditoolkit.com/streamer/demos/axis.html). Or discover even more
// utility functions [in the source](https://github.com/Gozala/streamer/blob/master/streamer.js)
