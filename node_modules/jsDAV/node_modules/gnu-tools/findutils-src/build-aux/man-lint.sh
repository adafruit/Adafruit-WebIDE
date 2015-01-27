#! /bin/sh

rv=0
srcdir="$1" ; shift

for manpage
do
  what="lint check on manpage $manpage"
  echo -n "$what: "
  messages="$( troff -t -man ${srcdir}/${manpage} 2>&1 >/dev/null )"
  if test -z "$messages" ; then
      echo "passed"
  else
      echo "FAILED:" >&2
      echo "$messages"     >&2
      rv=1
  fi
done
exit $rv
