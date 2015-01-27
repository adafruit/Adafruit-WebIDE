#!/bin/sh

tmpfiles=
trap 'rm -fr $tmpfiles' 1 2 3 15

p=t-yesno-
tmpfiles="${p}in.tmp ${p}xout.tmp ${p}out.tmp ${p}err.tmp"

# For now, only test with C locale
LC_ALL=C
export LC_ALL

# Test with seekable stdin; followon process must see remaining data
cat <<EOF > ${p}in.tmp
nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn - entire line consumed
yn - backspace does not change result
y
does not match either yesexpr or noexpr
n
EOF
cat <<EOF > ${p}xout.tmp
N
Y
Y
N
n
EOF
(./test-yesno${EXEEXT}; ./test-yesno${EXEEXT} 3; cat) \
  < ${p}in.tmp > ${p}out.tmp || exit 1
cmp ${p}xout.tmp ${p}out.tmp || exit 1

(./test-yesno${EXEEXT} 3; ./test-yesno${EXEEXT}; cat) \
  < ${p}in.tmp > ${p}out.tmp || exit 1
cmp ${p}xout.tmp ${p}out.tmp || exit 1

# Test for behavior on pipe
cat <<EOF > ${p}xout.tmp
Y
N
EOF
echo yes | ./test-yesno${EXEEXT} 2 > ${p}out.tmp || exit 1
cmp ${p}xout.tmp ${p}out.tmp || exit 1

# Test for behavior on EOF
cat <<EOF > ${p}xout.tmp
N
EOF
./test-yesno${EXEEXT} </dev/null > ${p}out.tmp || exit 1
cmp ${p}xout.tmp ${p}out.tmp || exit 1

# Test for behavior when stdin is closed
./test-yesno${EXEEXT} 0 <&- > ${p}out.tmp 2> ${p}err.tmp && exit 1
cmp ${p}xout.tmp ${p}out.tmp || exit 1
test -s ${p}err.tmp || exit 1

# Cleanup
rm -fr $tmpfiles

exit 0
