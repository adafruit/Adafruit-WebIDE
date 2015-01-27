# Changes #

## 0.2.1 / 2011-09-12 ##

  - Update documentation.
  - Improvements & tests for `tree` API.

## 0.2.0 / 2011-09-01 ##

  - Splitting library into two core and experimental modules.
  - Created clojure style aliases for 'head': 'first', 'pick' and 'tail':
    'rest'.
  - Exposing experimental `normilize` and `tree` APIs via
    'streamer/experimental' module.

## 0.1.1 / 2011-08-20 ##

  - Adding implementation of `stack`.
  - Adding implementation of `join`.
  - Some internal implementation simplifications.

## 0.1.0 / 2011-08-20 ##

  - Breaking API by changing argument order in filter / map / reduce functions.
    This style is more friendly when writing code in functional style. So that
    high order functions can be defined by currying.
    `var odds = map.bind(null, function($) { return !($%2) })`
  - Adding `take` function that is similar to `filter`, but resulting stream
    contains only first `n` elements that were not filtered out.

## 0.0.4 / 2011-06-27 ##

 - New function `cache` for caching computation intensive streams into memory.
 - Adding support for stream interruption by returning `false` from
   `next` callback.
 - New function `slice` & refactoring of `head` & `tail` on top of it.
 - New function `hub` for pub / sub.
 - Fixes for typos & grammar contributed by @kandelakig

## 0.0.3 / 2011-06-09 ##

  - Adding docs & demos.

## 0.0.2 / 2011-06-08 ##

  - Fixed git staging error that caused syntax error.

## 0.0.1 / 2011-06-08 ##

  - Initial release.
