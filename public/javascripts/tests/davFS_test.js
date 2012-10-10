$(function () {
  testMakeDirectory();

  function testMakeDirectory() {
    function callback(err, status) {
      console.log("testMakeDirectory");
      console.log("err", err);
      console.log("status", status);
      testMakeDirectoryFail();
    }
    davFS.mkDir('/filesystem/testDir', callback);
  }

  function testMakeDirectoryFail() {
    function callback(err, status) {
      console.log("testMakeDirectoryFail");
      console.log("err", err);
      console.log("status", status);
      testWriteFile();
    }
    davFS.mkDir('/filesystem/testDir', callback);
  }

  function testWriteFile() {
    function callback(err, status) {
      console.log("testWriteFile");
      console.log("err", err);
      console.log("status", status);
      testReadFile();
    }
    davFS.write("/filesystem/testDir/test.js", "var test = 13;", callback);
  }

  function testReadFile() {
    function callback(err, data) {
      console.log("testReadFile");
      console.log("err", err);
      console.log("data", data);
      testListDirectory();
    }
    davFS.read("/filesystem/testDir/test.js", callback);
  }

  function testListDirectory() {
    function callback(err, list) {
      console.log("testListDirectory");
      console.log("err", err);
      console.log("list", list);
      testCopyFile();
    }
    davFS.listDir('/filesystem', callback);
  }

  function testCopyFile() {
    function callback(err, status) {
      console.log("testCopyFile");
      console.log("err", err);
      console.log("status", status);
      testMoveFile();
    }
    davFS.copy("/filesystem/testDir/test.js", "/filesystem/testDir/test2.js", true, callback);
  }

  function testMoveFile() {
    function callback(err, status) {
      console.log("testMoveFile");
      console.log("err", err);
      console.log("status", status);
      testRemoveFile();
    }
    davFS.move("/filesystem/testDir/test.js", "/filesystem/testDir/test3.js", true, callback);
  }

  function testRemoveFile() {
    function callback(err, status) {
      console.log("testRemoveFile");
      console.log("err", err);
      console.log("status", status);
      testRemoveFileFail();
    }
    davFS.remove("/filesystem/testDir/test3.js", callback);
  }

  function testRemoveFileFail() {
    function callback(err, status) {
      console.log("testRemoveFileFail");
      console.log("err", err);
      console.log("status", status);
      testRemoveDirectory();
    }
    davFS.remove("/filesystem/testDir/test3.js", callback);
  }

  function testRemoveDirectory() {
    function callback(err, status) {
      console.log("testRemoveDirectory");
      console.log("err", err);
      console.log("status", status);
    }
    davFS.remove("/filesystem/testDir/", callback);
  }

});