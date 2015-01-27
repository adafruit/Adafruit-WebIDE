/**
 * @package node-sftp
 * @copyright  Copyright(c) 2011 Ajax.org B.V. <info AT ajax.org>
 * @author Mike de Boer <mike AT ajax DOT org>
 * @license http://github.com/ajaxorg/node-sftp/blob/master/LICENSE MIT License
 */

var Fs = require("fs");

/**
 * Generate a random uuid. Usage: Math.uuid(length, radix)
 *
 * EXAMPLES:
 *   // No arguments  - returns RFC4122, version 4 ID
 *   >>> Math.uuid()
 *   "92329D39-6F5C-4520-ABFC-AAB64544E172"
 *
 *   // One argument - returns ID of the specified length
 *   >>> Math.uuid(15)     // 15 character ID (default base=62)
 *   "VcydxgltxrVZSTV"
 *
 *   // Two arguments - returns ID of the specified length, and radix. (Radix must be <= 62)
 *   >>> Math.uuid(8, 2)  // 8 character ID (base=2)
 *   "01001010"
 *   >>> Math.uuid(8, 10) // 8 character ID (base=10)
 *   "47473046"
 *   >>> Math.uuid(8, 16) // 8 character ID (base=16)
 *   "098F4D35"
 *
 * @param {Number} [len]   The desired number of characters. Defaults to rfc4122, version 4 form
 * @param {Number} [radix] The number of allowable values for each character.
 * @type  {String}
 */
exports.uuid = function(len, radix) {
    var i,
        chars = exports.uuid.CHARS,
        uuid  = [],
        rnd   = Math.random;
    radix     = radix || chars.length;

    if (len) {
        // Compact form
        for (i = 0; i < len; i++)
            uuid[i] = chars[0 | rnd() * radix];
    }
    else {
        // rfc4122, version 4 form
        var r;
        // rfc4122 requires these characters
        uuid[8] = uuid[13] = uuid[18] = uuid[23] = "-";
        uuid[14] = "4";

        // Fill in random data.  At i==19 set the high bits of clock sequence as
        // per rfc4122, sec. 4.1.5
        for (i = 0; i < 36; i++) {
            if (!uuid[i]) {
                r = 0 | rnd() * 16;
                uuid[i] = chars[(i == 19) ? (r & 0x3) | 0x8 : r & 0xf];
            }
        }
    }

    return uuid.join("");
};
//Public array of chars to use
exports.uuid.CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz".split("");

exports.DEFAULT_TMPDIR = (function() {
    var value,
        def     = "/tmp",
        envVars = ["TMPDIR", "TMP", "TEMP"],
        i       = 0,
        l       = envVars.length;
    for(; i < l; ++i) {
        value = process.env[envVars[i]];
        if (value)
            return Fs.realpathSync(value).replace(/\/+$/, "");
    }
    return Fs.realpathSync(def).replace(/\/+$/, "");
})();

/**
 * Extends an object with one or more other objects by copying all their
 * properties.
 * @param {Object} dest the destination object.
 * @param {Object} src the object that is copies from.
 * @return {Object} the destination object.
 */
exports.extend = function(dest, src){
    var prop, i, x = !dest.notNull;
    if (arguments.length == 2) {
        for (prop in src) {
            if (x || src[prop])
                dest[prop] = src[prop];
        }
        return dest;
    }

    for (i = 1; i < arguments.length; i++) {
        src = arguments[i];
        for (prop in src) {
            if (x || src[prop])
                dest[prop] = src[prop];
        }
    }
    return dest;
};
