/*
	Gitty
	Author: Gordon Hall

	Copyright (c) 2012 Gordon Hall

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
	to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of
	the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
	THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

module.exports = (function() {
	// get required modules
	var fs = require('fs'),
			path = require('path'),
	    exec = require('child_process').exec,
	    back = __dirname;

	fs.existsSync || (fs.existsSync = path.existsSync);
	
	/*
	 * repository() - private
	 * checks the given path for git repo and if it exists, navigates to the directory
	 */
	function repository(path) {
		// does the path exist
		if (fs.existsSync(path)) {
			// get directory contents
			var contents = fs.readdirSync(path);
			// iterate over contents
			for (var item = 0; item < contents.length; item++) {
				// if the current item is .git
				if (contents[item] === '.git') {
					// move to dir
					process.chdir(path);
					console.log('"' + process.cwd() + '" is a valid Git repository. Switched to directory.');
					return true;
				// if current item is not .git
				} else {
					// if item is last in set
					if (contents[item] === contents[contents.length - 1]) {
						console.log('"' + path + '" is not a valid Git repository.')
						return false;
					}
				}
			}
		// return false if path invalid
		} else {
			return false;
		}
		process.chdir(back);
	}
	
	/*
	 * history() - public
	 * set process to run in specified directory and passes an array of commit objects into the callback
	 */
	function history(path, callback) {
		// create output string
		var output = '[',
		    command = '';
		// create bash command
			command += 'git log --pretty=format:\'{';
			command += '"commit": "%H",';
		 	command += '"author": "%an <%ae>",';
			command += '"date": "%ad",';
			command += '"message": "%s"';
			command += '},\'';
		// if the path is a valid repository, get the commit log
		if (repository(path)) {
			// execute bash command
			exec(command, function(err, stdout, stderr) {
				// if error, pass error object into callback
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						callback.call(this, {
							error : err || stderr
						});
					}
				// if successful, trim the trailing comma, and close output string
				// then parse the JSON object and pass into callback
				} else if (stdout) {
					output += stdout.substring(0, stdout.length - 1);
					output += ']';
					output = JSON.parse(output);
					if (callback) {
						process.chdir(back);
						callback.call(this, output);
					}
				}
			});
		} else {
			if (callback) {
				callback.call(this, {
					error : 'Invalid Repository'
				});
			}
		}
	}

	/*
	 * config() - public
	 * lists the git config and passes the val as a string into the callback
	 */
	function config(key, value, callback) {
		var command = 'git config --global';
		if (key) {
			command += ' ' + key;
		}

		if (value) {
			command += ' "' + value + '"';
		}
		exec(command, function(err, stdout, stderr) {
			console.log(err, stdout, stderr);
			// if error, pass error object into callback
			if (err || stderr) {
									console.log('here');
				console.log(err || stderr);
				if (callback) {
					callback.call(this, err || stderr, null);
				}
			// if successful, trim the trailing comma, and close output string
			// then parse the JSON object and pass into callback
			} else {
				var response;
				if (stdout) {
					response = stdout.replace('\n', '');
				}
				if (callback) {
					process.chdir(back);
					callback.call(this, null, response);
				}
			}
		});
	}
	
	/*
	 * create() - public
	 * creates a new directory, initializes a git repo, and adds a readme, then passes result into callback
	 */
	function create(name, description, path, callback) {
		// is path valid
		if (fs.existsSync(path)) {
			// get path to repos
			var repos = fs.readdirSync(path),
			// create initial readme text
			    readme_txt = '# ' + name + '\n\n' + description;
			// if the dir is not empty
			if (repos.length) {
				// iterate over items
				for (var repo = 0; repo < repos.length; repo++) {
					// does the repo already exist
					if (repos[repo] === name) {
						// pass error object into callback
						if (callback) {
							callback.call(this, {
								error : 'Directory already exists'
							});
						}
					// doesnt exist
					} else {
						// last in iteration
						if (repo === repos.length - 1) {
							// call ready()
							ready();
						}
					}
				}
			// otherwise go ahead
			} else {
				ready();
			}
			// called if all is good in the hood
			function ready() {
				// make sure the path format is good
				if (path.charAt(path.length - 1) !== '/') {
					path += '/';
				}
				// create the dir
				fs.mkdirSync(path + name);
				// move to the new dir
				process.chdir(path + name);
				// init git repo
				exec('git init', function(err, stdout, stderr) {
					// if error, pass error object into callback
					if (err || stderr) {
						console.log(err || stderr);
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								error : err || stderr
							});
						}
					// all is good
					} else if (stdout) {
						// create readme file and place text inside
						fs.writeFile(path + name + '/README.md', readme_txt, function() {
							// pass success object into callback
							var successMsg = stdout;
							add(path + name, ['README.md'], function(output) {
								if (callback) {
									callback.call(this, {
										success : successMsg
									});
								}
							});	
						});
					}
				});
			}
		// path isnt valid
		} else {
			// pass error into callback
			if (callback) {
				callback.call(this, {
					error : 'Invalid path'
				});
			}
		}
	}
	
	/*
	 * destroy() - public
	 * deletes git repo and optionally all contents
	 */
	function destroy(path, deletefiles, callback) {
		// if the path exists
		if (fs.existsSync(path)) {
			// make sure the path format is good
			if (path.charAt(path.length - 1) !== '/') {
				path += '/';
			}
			// if delete all contents
			if (deletefiles) {
				// force deletion of entire dir
				rmdirSyncForce(path);
				// pass success into callback
				if (callback) {
					callback.call(this, {
						success : 'Repository destroyed and contents deleted'
					});
				}
			// if just remove repo
			} else {
				// force deletion of .git dir
				rmdirSyncForce(path + '.git');
				// pass success into callback
				if (callback) {
					callback.call(this, {
						success : 'Repository destroyed and contents preserved'
					});
				}
			}
		// if not pass error into callback
		} else {
			if (callback) {
				callback.call(this, {
					error : 'Invalid path'
				});
			}
		}
		// force delete of dir and contents
		function rmdirSyncForce(path) {
			// init vars
			var files, file, fileStats, filesLength;
			// make sure the path format is good
			if (path.charAt(path.length - 1) !== '/') {
				path += '/';
			}
			// get files in dir
			files = fs.readdirSync(path);
			// get amount of files
			filesLength = files.length;
			// if files exists
			if (filesLength) {
				// iterate over files
				for (var i = 0; i < filesLength; i += 1) {
					file = files[i];
					// get file info
					fileStats = fs.statSync(path + file);
					// if its a file
					if (fileStats.isFile()) {
						// kill it
						fs.unlinkSync(path + file);
					}
					// if its a dir
					if (fileStats.isDirectory()) {
						// kill it and it's contents recursively
						rmdirSyncForce(path + file);
					}
				}
			}
			// delete now empty dir
			fs.rmdirSync(path);
		}
	}
	
	/*
	 * status() - public
	 * executes git status and parses into object containing staged/unstaged files
	 */
	function status(path, callback) {
		// move to repo
		if (repository(path)) {
			// execute git status parse
			exec('git status', function(err, stdout, stderr) {
				var status = stdout;
				exec('git ls-files --other --exclude-standard', function(err, stdout, stderr) {
					// if error, pass error object into callback
					if (err || stderr) {
						console.log(err || stderr);
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								error : err || stderr
							});
						}
					// all is good
					} else {
						process.chdir(back);
						callback.call(this, parseStatus(status, stdout));
					}
				});
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
		// status parser
		function parseStatus(gitstatus, untracked) {
			// create status object
			var status = {
				staged : [],
				not_staged : [],
				untracked : untracked.split('\n').slice(0, untracked.split('\n').length - 1)
			},
			// use this var to switch between arrays to push to
			file_status = null,
			// split output into array by line
			output = gitstatus.split('\n');
			// iterate over lines
			output.forEach(function(line) {
				// switch to staged array
				if (line.toLowerCase().indexOf('changes to be committed') > -1) {
					file_status = 'staged';
				// or switch to not_staged array
				} else if (line.toLowerCase().indexOf('changes not staged for commit') > -1) {
					file_status = 'not_staged';
				// or switch to untracked array
				} else if (line.toLowerCase().indexOf('untracked files') > -1) {
					file_status = 'untracked';
				}
				// check if the line contains a keyword
				if (line.indexOf('modified') > -1 ||
				    line.indexOf('new file') > -1 ||
				    line.indexOf('deleted') > -1) {
					// then remove # and all whitespace and split at the colon
					var fileinfo = line.substr(1).trim().split(':');
					// push a new object into the current array
					status[file_status].push({
						file : fileinfo[1].trim(),
						status : fileinfo[0]
					});
				}
			});
			return status;
		}
	}
	
	/*
	 * stage() - private
	 * adds or removes the array of files in the specified repository for commit
	 */
	function stage(operation, path, files, callback) {
		// move to repo
		if (repository(path)) {
			// store any errors
			var errs = [],
			    succs = [];
			if (!files.length) {
				callback.call(this, {
					errors : errs,
					added : succs
				});
			} else {
				console.log(files);
				// iterate over files
				files.forEach(function(val,key) {
					// execute git add for file
					exec('git ' + operation + ' ' + val, function(err, stdout, stderr) {
						// if error, push message into errs
						
						console.log(stdout);
						console.log(val);
						
						if (err || stderr) {
							errs.push(err || {
								file : val,
								error : stderr
							});
						// all is good
						} else {
							succs.push(val);
						}
						// if last in iteration pass output into callback
						if (val === files[files.length - 1]) {
							process.chdir(back);
							callback.call(this, {
								errors : errs,
								added : succs
							});
						}
					});
				});
			}
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * add() - public
	 * adds the array of files in the specified repository for commit
	 */
	function add(path, files, callback) {
		stage('add', path, files, callback);
	}
	
	/*
	 * remove() - public
	 * removes the array of files in the specified repository for commit
	 */
	function remove(path, files, callback) {
		stage('rm --cached', path, files, callback);
	}

	/*
	 * remove_recursive() - public
	 * removes the array of files in the specified repository for commit
	 */
	function remove_recursive(path, dir, callback) {
		if (repository(path)) {
			exec('git rm -r --cached ' + dir, function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else if (stdout) {
					process.chdir(back);
					callback.call(this, {
						message : stdout
					});
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}

	/*
	 * move() - public
	 * moves file or folder from source to destination.
	 */
	function move(path, source, destination, callback) {
		if (repository(path)) {
			var command = 'git mv ' + source + ' ' + destination;
			exec(command, function(err, stdout, stderr) {
				console.log(err, stdout, stderr);
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else {
					process.chdir(back);
					callback.call(this, {
						message : stdout
					});
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * unstage() - public
	 * removes the array of files in the specified repository for commit
	 */
	function unstage(path, files, callback) {
		stage('reset HEAD', path, files, callback);
	}
	
	/*
	 * commit() - public
	 * commits the current staged files to the working branch
	 */
	function commit(path, message, callback) {
		if (repository(path)) {
			exec('git commit -m ' + '"' + message + '"', function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else if (stdout) {
					process.chdir(back);
					callback.call(this, {
						message : stdout
					});
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * tree() - public
	 * gets the current branch that HEAD points to and passes to callback
	 */
	function tree(path, callback) {
		if (repository(path)) {
			exec('git branch', function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else if (stdout) {
					var tree = {
						current : null,
						others : []
					},
					branches = stdout.split('\n');
					branches.forEach(function(val, key) {
						if (val.indexOf('*') > -1) {
							tree['current'] = val.replace('*', '').trim();
						} else {
							if (val) {
								tree['others'].push(val.trim());
							}
						}
					});
					if (callback) {
						process.chdir(back);
						callback.call(this, tree);
					}
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * branch() - public
	 * creates a new branch for the specified repository
	 */
	function branch(path, branchname, callback) {
		if (repository(path)) {
			exec('git branch "' + branchname + '"', function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else {
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : 'Branch ' + branchname + ' created'
						});
					}
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * checkout() - public
	 * performs git checkout for the specified branch
	 */
	function checkout(path, branch, callback) {
		if (repository(path)) {
			exec('git checkout ' + branch, function(err, stdout, stderr) {

				if ((stdout) || stderr.indexOf('Switched') > -1) {
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : (stdout) || (stderr)
						});
					}
				} else if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				}  
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * merge - public
	 * merges the given branch insto the current branch
	 */
	function merge(path, branch, callback) {
		var cmd = 'git merge ' + branch;
		if (repository(path)) {
			exec(cmd, function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else {
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : stdout || 'Merged ' + branch + '!'
						});
					}
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * remote - public
	 * contains methods for adding removing and listing remotes
	 */
	var remote = (function() {
		
		/*
		 * add() - public
		 * adds a remote
		 */
		function add(path, remote, url, callback) {
			var cmd = 'git remote add ' + remote + ' ' + url;
			if (repository(path)) {
				exec(cmd, function(err, stdout, stderr) {
					if (err || stderr) {
						console.log(err || stderr);
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								error : err || stderr
							});
						}
					// all is good
					} else {
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								message : 'Remote "' + remote + '" added!'
							});
						}
					}
				});
			} else {
				callback.call(this, {
					error : 'Invalid repository'
				});
			}
		}
		
		/*
		 * update() - public
		 * edits a remote
		 */
		function update(path, remote, url, callback) {
			var cmd = 'git remote set-url ' + remote + ' ' + url;
			if (repository(path)) {
				exec(cmd, function(err, stdout, stderr) {
					if (err || stderr) {
						console.log(err || stderr);
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								error : err || stderr
							});
						}
					// all is good
					} else {
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								message : 'Remote "' + remote + '" updated!'
							});
						}
					}
				});
			} else {
				callback.call(this, {
					error : 'Invalid repository'
				});
			}
		}
		
		/*
		 * remove() - public
		 * removes a remote
		 */
		function remove(path, remote, callback) {
			var cmd = 'git remote rm ' + remote;
			if (repository(path)) {
				exec(cmd, function(err, stdout, stderr) {
					if (err || stderr) {
						console.log(err || stderr);
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								error : err || stderr
							});
						}
					// all is good
					} else {
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								message : 'Remote "' + remote + '" removed!'
							});
						}
					}
				});
			} else {
				callback.call(this, {
					error : 'Invalid repository'
				});
			}
		}
		
		/*
		 * list() - public
		 * lists remotes
		 */
		function list(path, callback) {
			var cmd = 'git remote -v';
			if (repository(path)) {
				exec(cmd, function(err, stdout, stderr) {
					if (err || stderr) {
						console.log(err || stderr);
						if (callback) {
							process.chdir(back);
							callback.call(this, {
								error : err || stderr
							});
						}
					// all is good
					} else {
						var list = {},
						    parseme = stdout.split('\n');
						
						parseme.forEach(function(val, key) {
							if (val.split('\t')[0])
							list[val.split('\t')[0]] = val.split('\t')[1].split(' ')[0];
						});

						if (callback) {
							process.chdir(back);
							callback.call(this, list);
						}
					}
				});
			} else {
				callback.call(this, {
					error : 'Invalid repository'
				});
			}
		}
		
		return {
			add : add,
			update : update,
			remove : remove,
			list : list
		}
	})();
	
	/*
	 * push() - public
	 * pushes the specified branch to the specified remote
	 */
	function push(path, remote, branch, callback) {
		var cmd = 'git push -u ' + ((remote) ? remote : '') + ' ' + ((branch) ? branch : '') + ' -q';
		if (repository(path)) {
			exec(cmd, function(err, stdout, stderr) {
				
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else if (stdout) {
										
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : stdout
						});
					}
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * pull() - public
	 * pulls the specified branch from the specified remote
	 */
	function pull(path, remote, branch, callback) {
		var cmd = 'git pull ' + ((remote) ? remote : '') + ' ' + ((branch) ? branch : '') + ' -q';
		if (repository(path)) {
			exec(cmd, function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else if (stdout) {
										
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : stdout
						});
					}
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * reset() - public
	 * resets the HEAD back to the specified commit hash
	 */
	function reset(path, hash, callback) {
		var cmd = 'git reset -q ' + hash;
		if (repository(path)) {
			exec(cmd, function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : err || stderr
						});
					}
				// all is good
				} else {
										
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : 'Repository reset to commit: ' + hash
						});
					}
				}
			});
		} else {
			callback.call(this, {
				error : 'Invalid repository'
			});
		}
	}
	
	/*
	 * clone() - public
	 * clones the given git url into the path specified
	 */
	function clone(path, url, callback) {
		var cmd = 'git clone ' + url;
		process.chdir(path);
		
		if (!url) {
			if (callback) {
				callback.call(this, {
					error : 'Please specify a URL'
				});
			}
		} else {
		
			exec(cmd, function(err, stdout, stderr) {
				if (err || stderr) {
					console.log(err || stderr);
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							error : stderr || 'Clone failed!'
						});
					}
				// all is good
				} else {
									
					if (callback) {
						process.chdir(back);
						callback.call(this, {
							message : stdout 
						});
					}
				}
			});
		}
	}
	
	// public methods
	return {
		history : history,
		config : config,
		create : create,
		destroy : destroy,
		status : status,
		add : add,
		remove : remove,
		remove_recursive : remove_recursive,
		move : move,
		commit : commit,
		tree : tree,
		branch : branch,
		checkout : checkout,
		unstage : unstage,
		remote : remote,
		push : push,
		pull : pull,
		reset : reset,
		merge : merge,
		clone : clone
	};
  
})();