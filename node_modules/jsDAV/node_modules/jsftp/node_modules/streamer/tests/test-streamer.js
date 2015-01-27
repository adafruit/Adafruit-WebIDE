/* vim:set ts=2 sw=2 sts=2 expandtab */
/*jshint asi: true newcap: true undef: true es5: true node: true devel: true
         forin: true */
/*global define: true */

!(typeof define === "undefined" ? function ($) { $(require, exports, module) } : define)(function (require, exports, module, undefined) {

'use strict';

exports['test list'] = require('./list')
exports['test map'] = require('./map')
exports['test filter'] = require('./filter')
exports['test reduce'] = require('./reduce')
exports['test zip'] = require('./zip')
exports['test head'] = require('./head')
exports['test tail'] = require('./tail')
exports['test append'] = require('./append')
exports['test merge'] = require('./merge')
exports['test hub'] = require('./hub')
exports['test cache'] = require('./cache')
exports['test stack'] = require('./stack')
exports['test join'] = require('./join')


if (module == require.main)
  require('test').run(exports)

});
