/**
 * @package node-sftp
 * @copyright  Copyright(c) 2011 Ajax.org B.V. <info AT ajax.org>
 * @author Mike de Boer <mike AT ajax DOT org>
 * @license http://github.com/ajaxorg/node-sftp/blob/master/LICENSE MIT License
 */

var Child_process = require("child_process");
var Fs = require("fs");
var Events = require("events")
var Util = require("./util");
var Ssh = require("./ssh");
var Socket = require("net").Socket;
var Tty = require("tty");

/**
 * @class Sftp object
 * @extends events.EventEmitter
 * 
 * @property {Number} state        state the Sftp instance is in
 * @property {String} activeBuffer buffer that contains the output of the active Sftp command
 * @property {Object} callbacks    hashmap that contains callbacks for all Sftp commands
 * @property {Array}  queue        simple FIFO queue for commands to hold until (re)connection
 * @property {Object} cmdOptions   list of options per Sftp command, fetched from the output of the help command
 * 
 * @param {Object}   options                    that contain all the info required 
 *                                              to setup a connection
 *                                              to a remote server. Structure:
 * @param {String}   options.username           Username on remote host.
 * @param {String}   [options.password]         Password on remote host, not required if private key is provided.
 * @param {String}   [options.privateKey]       Private key for username, not required if password is provided.
 * @param {String}   [options.host="localhost"] Hostname or IP to connect to. Default: 'localhost'
 * @param {Number}   [options.port=22]          SSH Port number. Default: 22
 * @param {Number}   [options.timeout=10000]    Inactivity timeout in milliseconds. Default: 10000ms
 * @param {Boolean}  [options.autoconnect=true] Connect on instantiation. Default: true
 * @param {String}   [options.exec]             Command to be executed right after a connection is established.
 * @param {Function} [cbconnect]                callback to invoke right after a connection is established.
 * @type  {Sftp}
 */
function Sftp(options, cbconnect) {
    Events.EventEmitter.call(this);

    this.options = Util.extend({
        host: "localhost",
        port: 22,
        timeout: 10000,
        autoconnect: true
    }, options || {});
    
    // initialize state variables
    this.state           = 0;
    this.activeCmdBuffer = "";
    this.callbacks       = {};
    this.queue           = [];
    this.cmdOptions      = {
        ls: []
    };
    
    var o     = this.options,
        _self = this;
    // plaintext private key needs to be written to file first
    if (o.privateKey && o.privateKey.indexOf("BEGIN RSA PRIVATE KEY") >= 0) {
        var _self = this;
        Ssh.writeKeyFile(o.privateKey, function(err, file) {
            if (err)
                return cbconnect(err);
            o.privateKey = file;
            _self.$privateKeyTemp = file;
            afterInit();
        });
    }
    else
        afterInit();

    function afterInit() {
        _self.state = Sftp.STATE_DISCONNECTED;
        _self.emit("ready");
        cbconnect = cbconnect || o.callback || K;

        if (o.exec) {
            var args = o.exec.split(" "),
                func = parts.shift(),
                cb   = o.callback || cbconnect;
            if (!_self[func])
                return cb("Unsupported method '" + func + "' specified in the exec option");
            _self.connect(cbconnect);
            args.push(cb);
            _self[func].apply(_self, args);
        }
        else if (_self.queue.length) {
            _self.connect(cbconnect);
            _self.exec.apply(_self, _self.queue.shift());
        }
        else if (o.autoconnect) {
            _self.connect(cbconnect);
        }
    }
}

/**
 * @constant
 */
Sftp.STATE_CONNECTED    = 0x0001;
/**
 * @constant
 */
Sftp.STATE_CONNECTING   = 0x0002;
/**
 * @constant
 */
Sftp.STATE_DISCONNECTED = 0x0004;

/**
 * @name Sftp.connect
 * @event
 */

/**
 * @name Sftp.disconnect
 * @event
 */

/**
 * @name Sftp.ready
 * @event
 */

/**
 * @name Sftp.data
 * @event
 * @param {String} e chunk of data that comes in while executing an Sftp command
 */

require("util").inherits(Sftp, Events.EventEmitter);

/** @lends Sftp */
(function() {
    this.activeCmd       = null;
    this.activeCmdBuffer = null;
    this.callbacks       = null;
    this.queue           = null;
    this.cmdOptions      = null;
    
    var K = function() {};
    
    /**
     * Setup an Sftp connection to a remote host
     * 
     * @param {Function} cbconnect callback to invoke right after a connection is established.
     * @type  {void}
     */
    this.connect = function(cbconnect) {
        if (!(this.state & Sftp.STATE_DISCONNECTED)) {
            if (cbconnect) {
                cbconnect(this.state & Sftp.STATE_CONNECTED 
                    ? null 
                    : this.state & Sftp.STATE_CONNECTING
                        ? "SFTP Error: already connecting to a host, please be patient" 
                        : "SFTP Error: invalid state."
                );
            }
            return;
        }

        this.state = Sftp.STATE_CONNECTING;

        var o     = this.options,
            args  = ["-o", "Port=" + o.port],
            _self = this;
        if (o.privateKey)
            args = args.concat(Ssh.buildArgs(o.privateKey));
        args.push(
            // first we push the correct hostname (appended with the path, if supplied)
            (o.username ? o.username + "@" : "") + o.host + (o.home ? ":" + o.home : "")
        );
        // push the connection string as argument:
        console.log("launching: sftp " + args.join(" "));
        var ps = Tty.open("sftp", args);
        this.socket = ps[0];
        this.child = ps[1];

        this.socket.on("data", function(data) {
            if (_self.state & Sftp.STATE_DISCONNECTED)
                _self.socket.destroy();
            parseReply.call(_self, data.toString(), 1);
        });

        this.child.on("exit", function(code) {
            _self.emit("disconnect", code);
            _self.socket.flush();
            _self.state = Sftp.STATE_DISCONNECTED;
            if (_self.$privateKeyTemp)
                Fs.unlink(_self.$privateKeyTemp);
        });

        this.callbacks["connect"] = function() {
            _self.state = Sftp.STATE_CONNECTED;

            send.call(_self, "help", "help", function(lines) {
                registerFeatures.call(_self, lines);
                _self.emit("connect");
                cbconnect && cbconnect();

                if (_self.queue.length)
                    _self.exec.apply(_self, _self.queue.shift());
            }, K);
        };
    };
    
    /**
     * Parse the output of a 'help' or '?' command and fetch the command options
     * that are supported by the remote server.
     * The supported options are cached and used by functions like ls()
     * 
     * @param {Array} lines Array of lines that contain strings of terminal output
     * @type  {void}
     * @private
     */
    function registerFeatures(lines) {
        lines = lines.slice(lines.indexOf("Available commands:") + 1);
        var _self = this;
        lines.forEach(function(line) {
            // parse the help output for a command
            var m = line.match(/^([^\t]*)[\s]{2,}([^\t]+)$/);
            if (!m || m.length != 3)
                return;
            // remove the unnecessary trailing spaces
            m[1] = m[1].replace(/[\s]+$/, "");
            if (!m[1])
                return;
            // parse the command structure to fetch the command options.
            // the regex is more generic then it must be, because I might use it
            // for something else later...
            m = m[1].match(/([^\s]*)(?:[\s]+)?([^\s]*)?(?:[\s]+)?([^\s]*)?(?:[\s]+)?([^\s]*)?/);
            if (m[2] && m[2].substr(0, 2) == "[-")
                _self.cmdOptions[m[1]] = m[2].substr(2, m[2].length - 3).split("");
        });
    }
    
    /**
     * Destroy an existing connection to remote host
     * 
     * @param {Function} cbdisconn callback to invoke right after the client disconnected
     * @type  {void}
     */
    this.disconnect = function(cbdisconn) {
        if (this.state & Sftp.STATE_DISCONNECTED)
            return cbdisconn();
        var _self = this;
        this.exec("bye", "bye", function(lines) {
            _self.state = Sftp.STATE_DISCONNECTED;
            _self.emit("disconnect");
            _self.child && _self.child.kill && _self.child.kill();
            _self.child = null;
            _self.socket && _self.socket.destroy();
            if (_self.$privateKeyTemp)
                Fs.unlink(_self.$privateKeyTemp, cbdisconn);
            else
                cbdisconn();
        });
    };
    
    /**
     * Change directory
     * 
     * @param {String}   path
     * @param {Function} cbcd
     * @type  {void}
     */
    this.cd = function(path, cbcd) {
        this.exec("cd", "cd " + (path || ""), function(lines) {
            cbcd(isError(lines));
        });
    };
    
    /**
     * Asynchronous chmod(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}       path
     * @param {String,octal} mode
     * @param {Function}     callback
     * @type  {void}
     */
    this.chmod = function(path, mode, callback) {
        if (typeof mode == "number")
            mode = mode.toString(8);
        this.exec("chmod", "chmod " + mode + " " + (path || ""), function(lines) {
            callback(isError(lines));
        });
    };
    
    /**
     * Asynchronous chown(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}   path
     * @param {String}   own
     * @param {Function} callback
     * @type  {void}
     */
    this.chown = function(path, own, cbchgrp) {
        this.exec("chown", "chown " + own + " " + (path || ""), function(lines) {
            cbchown(isError(lines));
        });
    };
    
    /**
     * Asynchronous symlink(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}   linkdata
     * @param {String}   path
     * @param {Function} callback
     * @type  {void}
     */
    this.symlink = function(linkdata, path, callback) {
        this.exec("ln", "ln " + linkdata + " " + path, function(lines) {
            callback(isError(lines));
        });
    };
    
    /**
     * Asynchronous mkdir(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}       path
     * @param {String,octal} mode
     * @param {Function}     callback
     * @type  {void}
     */
    this.mkdir = function(path, mode, callback) {
        if (!path)
            return callback("Invalid path.");
        var _self = this;
        this.exec("mkdir", "mkdir " + path, function(lines) {
            var err = isError(lines);
            if (err)
                return callback(err);
            _self.chmod(path, mode || 0755, cbmkdir);
        });
    };
    
    /**
     * Asynchronous pwd(1). The callback gets two arguments (err, path)
     * 
     * @param {Function} callback
     * @type  {void}
     */
    this.pwd = function(callback) {
        this.exec("pwd", "pwd", function(lines) {
            // getting back on stdin:
            // sftp> pwd 
            // Remote working directory: /home/cloud9
            callback(null, lines.join("").replace(/[^:]+:[\s]*([^\n\r\t]+)/g, "$1"));
        });
    };
    
    /**
     * Small and simple routine to transform a collection of output strings into
     * an object of property for each filesystem node.
     * 
     * @param {Array} lines
     * @type  {Object}
     * @private
     */
    function parseListing(lines) {
        var res = [];
        lines.forEach(function(line) {
            if (!line || /\/[\.]{1,2}$/.test(line))
                return;
            var m = line.match(/^[\s]*([drwx-]+)[\s]*([\d]+)[\s]*([\d]+)[\s]*([\d]+)[\s]*([\d]+)[\s]*([\w\s\d]*)([\d]{2}\:[\d]{2})[\s]*(.*)$/);
            if (!m)
                return;
            res.push({
                perms: m[1],
                uid: parseInt(m[3]),
                gid: parseInt(m[4]),
                size: parseInt(m[5]),
                date: m[6],
                time: m[7],
                path: m[8]
            });
        });
        return res;
    }

    /**
     * Execute the 'ls' command, which is used for {@link Sftp#readdir}, {@link Sftp#stat}, 
     * {@link Sftp#fstat} and {@link Sftp#lstat} respectively. For {@link Sftp#stat} 
     * and its siblings, using 'ls' is the only way to retrieve a filesystem node's
     * stats.
     * Important: 'ls' output results are cached for 10 seconds, to improve performance.
     * 
     * @param {String}   path
     * @param {Function} cbls
     * @type  {void}
     * @private
     */
    function ls(path, cbls) {
        if (!this.$lsCache)
            this.$lsCache = {};
        var cache = this.$lsCache[path],
            now   = Date.now(),
            _self = this;
        if (cache) {
            if (cache.expires >= now)
                return cbls(null, cache.result);
            else
                delete this.$lsCache[path];
        }

        var cmd = "ls -l";
        if (this.cmdOptions["ls"].indexOf("a") > -1)
            cmd += "a";
        if (this.cmdOptions["ls"].indexOf("n") > -1)
            cmd += "n";
        this.exec("ls", cmd + " " + (path || ""), function(lines) {
            var err = isError(lines),
                res = parseListing(lines);
            if (!err) {
                _self.$lsCache[path] = {
                    expires: now + 10000,
                    result: res
                };
            }
            cbls(err, res);
        });
    }
    
    /**
     * Asynchronous readdir(3). Reads the contents of a directory. The callback 
     * gets two arguments (err, files) where files is an array of the names of 
     * the files in the directory excluding '.' and '..'.
     * 
     * @param {String}   path
     * @param {Function} callback
     * @type  {void}
     */
    this.readdir = function(path, callback) {
        ls.call(this, path, function(err, listing) {
            if (err)
                return callback(err);

            callback(null, listing.map(function(item) {
                var p = item.path;
                return p.substr(p.lastIndexOf("/") + 1);
            }));
        });
    };
    
    /**
     * Asynchronously reads the entire contents of a file.
     * The callback is passed two arguments (err, data), where data is the contents 
     * of the file.
     * If no encoding is specified, then the raw buffer is returned.
     * Example:
     * <pre class="code">
     * sftp.readFile("/etc/passwd", function(err, data) {
     *     if (err)
     *         throw err;
     *     console.log(data);
     * });
     * </pre>
     * 
     * @param {String}   filename
     * @param {String}   encoding
     * @param {Function} callback
     * @type  {void}
     */
    this.readFile = function(filename, encoding, callback) {
        var temp = Util.DEFAULT_TMPDIR + "/" + Util.uuid();

        this.exec("readFile", "get " + filename + " " + temp, function(lines) {
            var err = isError(lines);
            if (err)
                return callback(err);
            Fs.readFile(temp, encoding, function(err, data) {
                if (err)
                    return callback(err);
                Fs.unlink(temp, function() {
                    // error? we don't care here...
                    callback(null, data);
                });
            });
        });
    };
    
    /**
     * Asynchronous rename(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}   path1
     * @param {String}   path2
     * @param {Function} callback
     * @type  {void}
     */
    this.rename = function(path1, path2, callback) {
        this.exec("rename", "rename " + path1 + " " + (path2 || path1), function(lines) {
            callback(isError(lines));
        });
    };
    
    /**
     * Asynchronous rmdir(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}   path
     * @param {Function} callback
     * @type {void}
     */
    this.rmdir = function(path, callback) {
        this.exec("rmdir", "rmdir " + path, function(lines) {
            callback(isError(lines));
        });
    };
    
    /**
     * Helper function to get an octal representation from terminal output in a 
     * human readable form.
     * Example: 'drwxr-xr-x' --> '755' or '-rw-rw-r--' --> '664'
     * 
     * @param {String} rwx
     * @type  {String}
     * @private
     */
    function getOct(rwx) {
        var oct = "";
        for (var c, o = 0, i = 0, l = rwx.length; i < l; ++i) {
            c = rwx.charAt(i);
            if (i % 3 === 0) {
                oct += "" + o;
                o = 0;
            }
            c = rwx.charAt(i + 1);
            o += c == "r" ? 4 : c == "w" ? 2 : c == "x" ? 1 : 0;
        }
        return oct;
    }
    
    /**
     * Sftp specific Stat object
     * @class Objects returned from sftp.stat() and sftp.lstat() are of this type.
     *  
     * @property {Number} uid
     * @property {Number} gid
     * @property {String} mtime
     * @property {Number} size
     * @property {String} mode
     */
    var Stat = Sftp.Stat = function(struct) {
        this.uid   = struct.uid;
        this.gid   = struct.gid;
        this.mtime = struct.date + struct.time;
        this.size  = struct.size;
        this.mode  = parseInt(getOct(struct.perms), 10);
        
        /**
         * @type {Boolean}
         */
        this.isFile = function() {
            return struct.perms.charAt(0) == "-";
        };
        /**
         * @type {Boolean}
         */
        this.isDirectory = function() {
            return struct.perms.charAt(0) == "d";
        };
        /**
         * @type {Boolean}
         */
        this.isBlockDevice = function() {
            return struct.perms.charAt(0) == "b";
        };
        /**
         * @type {Boolean}
         */
        this.isCharacterDevice = function() {
            return struct.perms.charAt(0) == "c";
        };
        /**
         * @type {Boolean}
         */
        this.isSymbolicLink = function() {
            return struct.perms.charAt(0) == "l";
        };
        /**
         * @type {Boolean}
         */
        this.isFIFO = function() {
            return struct.perms.charAt(0) == "p";
        };
        /**
         * @type {Boolean}
         */
        this.isSocket = function() {
            return struct.perms.charAt(0) == "s";
        };
    };
    
    
    /** 
     * Alias of {@link Sftp#stat}
     * @function
     */
    this.fstat =
    /**
     * Alias of {@link Sftp#stat}
     * @function
     */
    this.lstat = this.stat;
    /**
     * Asynchronous lstat(2). The callback gets two arguments (err, stats) where 
     * stats is a sftp.Stats object. lstat() is identical to stat(), except that if 
     * path is a symbolic link, then the link itself is stat-ed, not the file that 
     * it refers to.
     * 
     * @param {String}   path
     * @param {Function} callback
     * @type  {void}
     */
    this.stat  = function(path, callback) {
        var _self = this,
            parts = path.split("/"),
            node  = parts.pop(),
            root  = parts.join("/");
        if (root.charAt(0) != "/") {
            this.pwd(function(err, pwd) {
                if (err)
                    return callback(err);
                pwd = pwd.replace(/[\/]+$/, "");
                root = pwd + "/" + root.replace(/^[\/]+/, "");
                afterPwd();
            });
        }
        else
            afterPwd();
        
        function afterPwd() {
            ls.call(_self, root, function(err, list) {
                list = list.filter(function(item) {
                    return item.path.split("/").pop() === node;
                });
                if (list.length === 0)
                    return callback("Couldn't stat remote file: No such file or directory");
                return callback(null, new Stat(list[0]));
            });
        }
    };
    
    /**
     * Asynchronous unlink(2). No arguments other than a possible exception are 
     * given to the completion callback.
     * 
     * @param {String}   path
     * @param {Function} callback
     * @type  {void}
     */
    this.unlink = function(path, callback) {
        this.exec("unlink", "rm " + path, function(lines) {
            callback(isError(lines));
        });
    };
    
    /**
     * Asynchronously writes data to a file. data can be a string or a buffer.
     * Example:
     * <pre class="code">
     * sftp.writeFile("message.txt", "Hello Node", function(err) {
     *     if (err)
     *         throw err;
     *     console.log("It's saved!");
     * });
     * </pre>
     * 
     * @param {String}        filename
     * @param {String,Buffer} data
     * @param {String}        encoding
     * @param {Function}      callback
     * @type  {void}
     */
    this.writeFile = function(filename, data, encoding, callback) {
        encoding  = encoding || "utf8";
        var temp  = Util.DEFAULT_TMPDIR + "/" + Util.uuid(),
            _self = this;

        Fs.writeFile(temp, data, encoding, function(err) {
            if (err)
                return callback(err);
            
            _self.exec("writeFile", "put " + temp + " " + filename, function(lines) {
                Fs.unlink(temp, function() {
                    var err = isError(lines);
                    if (err)
                        return callback(err);
                    callback();
                });
            });
        });
    };
    
    /**
     * Prepare an Sftp command to be sent to the remote host
     * 
     * @param {String}   type
     * @param {String}   cmd
     * @param {Function} cbexec
     * @param {Function} cbprogress
     * @type  {void}
     */
    this.exec = function(type, cmd, cbexec, cbprogress) {
        var conn = this.state & Sftp.STATE_CONNECTED;
        if (this.activeCmd || !conn) {
            if (!conn)
                this.connect();
            return this.queue.push([type, cmd, cbprogress, cbexec]);
        }
        
        send.call(this, type, cmd, cbexec, cbprogress || K);
    };
    
    /**
     * Send an Sftp command to the remote host, i.e. write it to the CLI stream
     * 
     * @param {String}   type
     * @param {String}   cmd
     * @param {Function} cbsend
     * @param {Function} cbprogress
     * @type  {void}
     * @private
     */
    function send(type, cmd, cbsend, cbprogress) {
        this.activeCmd = type;
        this.activeCmdBuffer = "";
        if (cbprogress && cbsend) {
            this.callbacks[type] = cbsend;
            this.callbacks[type + "_progress"] = cbprogress;
        }
        this.socket.write(new Buffer(cmd + "\r\n"));
        this.socket.resume();
    }
    
    /**
     * Parse the STDOUT of the sftp child process, collect the lines of data for
     * the active command and detect if the active command has finished executing.
     * If the active command is done, respective callbacks are called.
     * 
     * @param {String} data
     * @type  {void}
     * @private
     */
    function parseReply(data) {
        // console.log("data: ", data, data.split("\n").length + " parts");
        this.emit("data", data);
        if (!this.activeCmd && !(this.state & Sftp.STATE_CONNECTING))
            return;
        var cbdone, cbprogress;
        if (data.indexOf("sftp>") > -1 || (this.activeCmd == "bye" && data.indexOf("bye") > -1)) {
            if (this.state & Sftp.STATE_CONNECTING && this.callbacks["connect"]) {
                this.callbacks["connect"]();
                delete this.callbacks["connect"];
            }
            // check if a command has finished executing:
            else if (cbdone = this.callbacks[this.activeCmd]) {
                delete this.callbacks[this.activeCmd];
                delete this.callbacks[this.activeCmd + "_progress"];
                this.activeCmd = null;
                cbdone((this.activeCmdBuffer + data).split(/[\n\r]+/).filter(function(line) {
                    return line.indexOf("sftp>") === -1;
                }));
                this.activeCmdBuffer = "";
            }
            if (!this.activeCmd && this.queue.length && this.state & Sftp.STATE_CONNECTED)
                this.exec.apply(this, this.queue.shift());
        }
        else if (cbprogress = this.callbacks[this.activeCmd + "_progress"]) {
            this.activeCmdBuffer += data;
            cbprogress(data);
        }
    }
    
    /**
     * Detect an error reply from a collection of output lines.
     * 
     * @param {Array} lines
     * @private
     */
    function isError(lines) {
        var err = null;
        lines.forEach(function(line) {
            if (line.indexOf("No such file or directory") > -1)
                err = line;
        });
        return err;
    }
}).call(Sftp.prototype);

module.exports = Sftp;
