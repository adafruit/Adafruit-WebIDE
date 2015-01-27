install: pcre bin/grep bin/find

clean:
	rm -r bin	
	cd findutils-src && make distclean
	cd grep-src && make distclean

bin:
	mkdir bin

pcre:
	cd pcre-src && ./configure && make
	mkdir -p bin && cp pcre-src/.libs/pcregrep bin

bin/find: bin 
	cd findutils-src && ./configure && make
	cp findutils-src/find/find bin/find

bin/grep: bin
	cd grep-src && ./configure --with-pcre=../pcre-src && make
	cp grep-src/src/grep bin/grep
	
publish: clean
	npm publish