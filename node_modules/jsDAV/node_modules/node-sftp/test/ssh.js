/**
 * @package node-sftp
 * @subpackage test
 * @copyright  Copyright(c) 2011 Ajax.org B.V. <info AT ajax.org>
 * @author Mike de Boer <mike AT ajax DOT org>
 * @license http://github.com/ajaxorg/node-sftp/blob/master/LICENSE MIT License
 */

var assert = require("assert");
var ssh = require("./../lib/ssh");
var secrets = require("./secrets");

module.exports = {
    
    timeout: 10000,
    
    setUp : function(next) {
        next();
    },
    
    tearDown : function(next) {        
        next();
    },
    
    "test spawn command over ssh with a custom private key": function(next) {
        ssh.spawn(secrets.prvkey, secrets.host, "pwd", [], function(err, ls) {
            var out = err = "";
            
            ls.on("exit", function(code) {
                //console.log(code);
                assert.equal(out, "/home/sshtest\n");
                next();
            });
                
            ls.stdout.on("data", function (data) {
                //console.log("out: " + data);
                out += data;
            });
    
            ls.stderr.on("data", function (data) {
                //console.log("err: " + data);
                err += data;
            });
        });
    },
    
    "test exec command over ssh with a custom private key": function(next) {
        ssh.exec(secrets.prvkey, secrets.host, "pwd", [], function(code, stdout, stderr) {
            assert.equal(stdout, "/home/sshtest\n");
            next();
        });
    },
    
    "test generate key pair": function(next) {
        ssh.generateKeyPair("fabian@ajax.org", function(err, pub, prv) {
            assert.equal(err, null);
            assert.ok(pub.indexOf("fabian@ajax.org") >= 0);
            assert.ok(prv.indexOf("PRIVATE KEY") >= 0);
            next();
        })
    },
    
    "test validate ssh key and repo has key" : function(next) {
        ssh.validateSSHKey(secrets.githubPrvkey, "git@github.com", function(err, hasKey) {
            assert.equal(err, null);
            assert.ok(hasKey);
            next();
        })
    },
    
    "test validate ssh key and repo doesn't have the key" : function(next) {
        ssh.validateSSHKey(secrets.unusedKey, "git@github.com", function(err, hasKey) {
            assert.equal(err, null);
            assert.ok(!hasKey);
            next();
        })
    },
    
    "test validate ssh key and repo doesn't have the key2" : function(next) {
        ssh.validateSSHKey(secrets.unusedKey, "cloud9@c9.io", function(err, hasKey) {
            assert.equal(err, null);
            assert.ok(!hasKey);
            next();
        })
    }
}

!module.parent && require("./../../async.js/lib/test").testcase(module.exports, "SSH Env").exec();
