#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="$tmpfiles t-can.tmp ise"
mkdir t-can.tmp
ln -s t-can.tmp/ket ise \
  || { echo "Skipping test: symbolic links not supported on this filesystem"
       rm -fr $tmpfiles
       exit 77
     }
(cd t-can.tmp \
 && ln -s bef plo \
 && ln -s tra huk \
 && ln -s lum bef \
 && ln -s wum ouk \
 && ln -s ../ise ket \
 && echo > tra \
 && mkdir lum
) || exit 1

# Trigger a bug that would make the function mistakenly report a loop.
# To trigger it, we have to construct a name/situation during the
# resolution of which the code dereferences the same symlink (S)
# two different times with no actual loop.  In addition, the
# second and fourth calls to readlink must operate on S.
(cd t-can.tmp \
 && ln -s s p \
 && ln -s d s \
 && mkdir d \
 && echo > d/2 \
 && ln -s ../s/2 d/1
) || exit 1

./test-canonicalize${EXEEXT}
result=$?

rm -fr $tmpfiles

exit $result
