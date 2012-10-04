var fs_helper = require('../editor/helpers/fs_helper'),
    fs = require('fs'),
    should = require('should');

describe('fs_helper', function() {
  describe('ssh keys', function(){
    before(function(){
      try {
        fs.unlinkSync(process.env.HOME + '/.ssh/id_rsa_bitbucket');
        fs.unlinkSync(process.env.HOME + '/.ssh/id_rsa_bitbucket.pub');
      } catch(e) {

      }
    });    

    describe('when the key does not exist', function(){
      it('should be false', function(done){
        fs_helper.has_ssh_key(function(exists) {
          exists.should.equal(false);
          done();
        });
      });
    });

    describe('when the key is generated', function(){
      it('it should succeed', function(done){
        fs_helper.generate_ssh_key(done);
      });
    });


    describe('when the key exists', function(){
      it('should be true', function(done){
        fs_helper.has_ssh_key(function(exists) {
          exists.should.equal(true);
          done();
        });
      });
    });
  });

  describe('check_for_repository', function() {
    //TODO: test positive case
    it('repository should not exist', function(done) {
      fs_helper.check_for_repository('false-repository', function(err, exists) {
        should.not.exist(err);
        exists.should.equal(false);
        done();
      });
    });
  });
});