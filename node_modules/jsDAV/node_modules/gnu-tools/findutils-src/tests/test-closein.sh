#!/bin/sh

tmpfiles=
trap 'rm -fr $tmpfiles' 1 2 3 15

p=t-closein-
tmpfiles="${p}in.tmp ${p}xout.tmp ${p}out1.tmp ${p}out2.tmp"

echo Hello world > ${p}in.tmp
echo world > ${p}xout.tmp

# Test with seekable stdin; followon process must see remaining data
(./test-closein${EXEEXT}; cat) < ${p}in.tmp > ${p}out1.tmp || exit 1
cmp ${p}out1.tmp ${p}in.tmp || exit 1

(./test-closein${EXEEXT} consume; cat) < ${p}in.tmp > ${p}out2.tmp || exit 1
cmp ${p}out2.tmp ${p}xout.tmp || exit 1

# Test for lack of error on pipe
cat ${p}in.tmp | ./test-closein${EXEEXT} || exit 1

cat ${p}in.tmp | ./test-closein${EXEEXT} consume || exit 1

# Test for lack of error when nothing is read
./test-closein${EXEEXT} </dev/null || exit 1

./test-closein${EXEEXT} <&- || exit 1

# Test for no error when EOF is read early
./test-closein${EXEEXT} consume </dev/null || exit 1

# Test for error when read fails because no file available
./test-closein${EXEEXT} consume close <&- 2>/dev/null && exit 1

# Cleanup
rm -fr $tmpfiles

exit 0
