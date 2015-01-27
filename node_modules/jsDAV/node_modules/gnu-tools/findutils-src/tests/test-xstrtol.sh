#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="t-xstrtol.tmp t-xstrtol.xo"
: > t-xstrtol.tmp
too_big=99999999999999999999999999999999999999999999999999999999999999999999
result=0

# test xstrtol
./test-xstrtol${EXEEXT} 1 >> t-xstrtol.tmp 2>&1 || result=1
./test-xstrtol${EXEEXT} -1 >> t-xstrtol.tmp 2>&1 || result=1
./test-xstrtol${EXEEXT} 1k >> t-xstrtol.tmp 2>&1 || result=1
./test-xstrtol${EXEEXT} ${too_big}h >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtol${EXEEXT} $too_big >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtol${EXEEXT} x >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtol${EXEEXT} 9x >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtol${EXEEXT} 010 >> t-xstrtol.tmp 2>&1 || result=1
# suffix without integer is valid
./test-xstrtol${EXEEXT} MiB >> t-xstrtol.tmp 2>&1 || result=1

# test xstrtoul
./test-xstrtoul${EXEEXT} 1 >> t-xstrtol.tmp 2>&1 || result=1
./test-xstrtoul${EXEEXT} -1 >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtoul${EXEEXT} 1k >> t-xstrtol.tmp 2>&1 || result=1
./test-xstrtoul${EXEEXT} ${too_big}h >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtoul${EXEEXT} $too_big >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtoul${EXEEXT} x >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtoul${EXEEXT} 9x >> t-xstrtol.tmp 2>&1 && result=1
./test-xstrtoul${EXEEXT} 010 >> t-xstrtol.tmp 2>&1 || result=1
./test-xstrtoul${EXEEXT} MiB >> t-xstrtol.tmp 2>&1 || result=1

# normalize output
sed -e 's/^[^:]*: //' < t-xstrtol.tmp > t-xstrtol.xo
mv t-xstrtol.xo t-xstrtol.tmp

# compare expected output
cat > t-xstrtol.xo <<EOF
1->1 ()
-1->-1 ()
1k->1024 ()
invalid suffix in X argument \`${too_big}h'
X argument \`$too_big' too large
invalid X argument \`x'
invalid suffix in X argument \`9x'
010->8 ()
MiB->1048576 ()
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

diff t-xstrtol.xo t-xstrtol.tmp || result=1

rm -fr $tmpfiles

exit $result
