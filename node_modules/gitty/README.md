# Gitty

Gitty is a Node.js module that acts as a wrapper for the Git CLI. It uses methods that resemble the Git command line syntax to asynchronously execute common commands, and return passes the output as standard JavaScript objects and arrays - depending on the call.

## Prerequisites

* Node.js 0.8.x (http://nodejs.org)
* Git 1.7.x.x (http://git-scm.com)

## Installation

```
$ npm install gitty
```

## Usage

```javascript
var git = require('gitty');

// get commit log as an array of objects
// and log to the console
git.history('/path/to/repo', function(output) {
	console.log(output);
});
```

## Documentation

Internally, Gitty uses a private method `repository(path)` that checks if the given path is a valid Git repository. If the check passes, then a child process is spawned and the process moves into the given directory to perform the command. After the callback is fired for the given function, Gitty moves the process back to it's original directory.

> To ensure the best performance you should always use absolute or server-relative paths. For example '/users/username/repositories/projectname'.

All `path` arguments should follow this convention.

Callbacks for all methods are given a single object. Usually, this object contains a single property and value. **Successful** operations get a `message` property containing either the `stdout` from the command or a message confirming the operation was successfull.

**Failed** operations get an `error` property, for which the value is either one of: an error from Node, `stderr` from the operation, or a message alert the operation failed - whichever is relevant.

This is not the case for every method. Refer to the method list below for specifics.

### history(path, [callback])

Gives an *array* of objects containing `commit`, `author`, `date`, and `message`.

### create(name, description, path, [callback])

Creates a directory at the given path, create a README markdown file with the name and desciption, initializes a git repository, and stages an initial commit.

### destroy(path, deletefiles, [callback])

Uninitializes the repository at the given path. The `deletefiles` argument is a boolean that determines whether of not to also delete the contents of the repository.

### status(path, [callback])

Gives an object with properties for `staged`, `not_staged`, and `untracked`. Each property represents an array of objects - each containing a `file` and `status`. 

### add(path, files, [callback])

Adds an array of files for commit, and gives back and object of `errors` (array) and `added` (array).

### remove(path, files, [callback])

Removes array of files for commit, and gives back and object of `errors` (array) and `added` (array).

### unstage(path, files, callback)

Unstages array of files for commit, and gives back and object of `errors` (array) and `added` (array).

### commit(path, message, [callback])

Stages a commit with message based on the current staged files.

### tree(path, [callback])

Gives back an object representing the `current` branch and an array of `others`.

### branch(path, branchname, [callback])

Creates a branch using the `branchname` for the repository at the given path

### checkout(path, branchname, [callback])

Does a checkout on the given `branchname`.

### remote.add(path, remote, url, callback)

Adds a remote to the given repository.

### remote.update(path, remote, url, callback)

Updates an existing remote's url.

### remote.remove(path, remote, callback)

Removes the given remote from the repository.

### remote.list(path, [callback])

Returns an object where each `key` is the remote name and `val` is the remote url.

### push(path, remote, branch, callback)

Pushes the passed `branch` to the passed `remote`.

### pull(path, remote, branch, callback)

Pulls the passed `branch` from the passed `remote`.

### reset(path, hash, callback)

Resets the HEAD back to the status of the passed commit hash.

### merge(path, branch, [callback])

Merges the given `branch` with the current branch.

### clone(path, url, callback)

Clones a remote repository into the specified path.

## Author
Gitty was written by Gordon Hall (gordon@gordonwritescode.com)  
Licensed under MIT license