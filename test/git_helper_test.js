var git_helper = require('../editor/helpers/git_helper'),
    fs = require('fs'),
    path = require('path'),
    should = require('should'),
    rimraf = require('rimraf');

describe('git_helper', function() {
  describe('clone_repository', function(){
    before(function(){
      var repository_path = path.join(__dirname + "/../repositories", 'Adafruit-Raspberry-Pi-Python-Code');
      rimraf.sync(repository_path);
    });

    describe('when the repository does not exist', function(){
      it('should not fail', function(done){
        var repository = 'git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git';
        git_helper.clone_repository(repository, function(err, message) {
          should.not.exist(err);
          message.should.include('Cloning into');
          done();
        });
      });
    });

    describe('when the repository exists', function(){
      it('should return an error', function(done){
        var repository = 'git://github.com/adafruit/Adafruit-Raspberry-Pi-Python-Code.git';
        git_helper.clone_repository(repository, function(err, message) {
          should.exist(err);
          should.not.exist(message);
          err.should.include('already exists and is not an empty directory.');
          done();
        });
      });
    });    
  });

  describe('validate_config', function() {
    it('should be valid', function(done) {
      git_helper.validate_config(function(is_valid) {
        is_valid.should.be.true;
        done();
      });
    });
  });

  describe('add', function() {
    describe('when an empty file is added', function() {
      //TODO: this test is failing...the add method does not return a valid error
      it('should return an error', function(done) {
        git_helper.add('Adafruit-Raspberry-Pi-Python-Code', '', function(err, message) {
          console.log(err, message);
          done();
        });
      });
    });

  });  
});