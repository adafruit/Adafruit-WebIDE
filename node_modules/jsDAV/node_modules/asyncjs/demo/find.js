#!/usr/bin/env node

var async = require("../index")

function find(path, callback) {    
    async.walkfiles(path, null, async.PREORDER)
        .each(function(file) {
            console.log(file.path)
        })
        .end(callback)
}

find(process.argv[2] || ".")