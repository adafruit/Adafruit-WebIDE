#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="t-xstrtoumax.tmp t-xstrtoumax.xo"
: > t-xstrtoumax.tmp
too_big=99999999999999999999999999999999999999999999999999999999999999999999
result=0

# test xstrtoumax
./test-xstrtoumax${EXEEXT} 1 >> t-xstrtoumax.tmp 2>&1 || result=1
./test-xstrtoumax${EXEEXT} -1 >> t-xstrtoumax.tmp 2>&1 && result=1
./test-xstrtoumax${EXEEXT} 1k >> t-xstrtoumax.tmp 2>&1 || result=1
./test-xstrtoumax${EXEEXT} ${too_big}h >> t-xstrtoumax.tmp 2>&1 && result=1
./test-xstrtoumax${EXEEXT} $too_big >> t-xstrtoumax.tmp 2>&1 && result=1
./test-xstrtoumax${EXEEXT} x >> t-xstrtoumax.tmp 2>&1 && result=1
./test-xstrtoumax${EXEEXT} 9x >> t-xstrtoumax.tmp 2>&1 && result=1
./test-xstrtoumax${EXEEXT} 010 >> t-xstrtoumax.tmp 2>&1 || result=1
./test-xstrtoumax${EXEEXT} MiB >> t-xstrtoumax.tmp 2>&1 || result=1

# normalize output
sed -e 's/^[^:]*: //' < t-xstrtoumax.tmp > t-xstrtoumax.xo
mv t-xstrtoumax.xo t-xstrtoumax.tmp

# compare expected output
cat > t-xstrtoumax.xo <<EOF
1->1 ()
invalid X argument \`-1'
1k->1024 ()
invalid suffix in X argument \`${too_big}h'
X argument \`$too_big' too large
invalid X argument \`x'
invalid suffix in X argument \`9x'
010->8 ()
MiB->1048576 ()
EOF

diff t-xstrtoumax.xo t-xstrtoumax.tmp || result=1

rm -fr $tmpfiles

exit $result
