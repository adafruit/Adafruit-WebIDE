#!/bin/sh

tmpfiles=""
trap 'rm -fr $tmpfiles' 1 2 3 15

tmpfiles="$tmpfiles t-can-lgpl.tmp ise"
mkdir t-can-lgpl.tmp
ln -s t-can-lgpl.tmp/ket ise \
  || { echo "Skipping test: symbolic links not supported on this filesystem"
       rm -fr $tmpfiles
       exit 77
     }
(cd t-can-lgpl.tmp \
 && ln -s bef plo \
 && ln -s tra huk \
 && ln -s lum bef \
 && ln -s wum ouk \
 && ln -s ../ise ket \
 && echo > tra \
 && mkdir lum
) || exit 1

./test-canonicalize-lgpl${EXEEXT}
result=$?

rm -fr $tmpfiles

exit $result
