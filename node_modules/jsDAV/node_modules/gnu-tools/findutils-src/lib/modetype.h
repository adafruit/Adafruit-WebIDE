/* modetype.h -- file type bits definitions for POSIX systems
   Requires sys/types.h sys/stat.h.
   Copyright (C) 1990, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* POSIX.1 doesn't mention the S_IFMT bits; instead, it uses S_IStype
   test macros.  To make storing file types more convenient, define
   them; the values don't need to correspond to what the kernel uses,
   because of the way we use them. */
#ifndef INC_MODETYPE_H
#define INC_MODETYPE_H 1

#ifndef S_IFMT			/* Doesn't have traditional Unix macros. */
#define S_IFBLK 1
#define S_IFCHR 2
#define S_IFDIR 4
#define S_IFREG 8
#ifdef S_ISLNK
#define S_IFLNK 16
#endif
#ifdef S_ISFIFO
#define S_IFIFO 32
#endif
#ifdef S_ISSOCK
#define S_IFSOCK 64
#endif
#ifdef S_ISDOOR
#define S_IFDOOR 128
#endif
#endif /* !S_IFMT */

#ifdef STAT_MACROS_BROKEN
#undef S_ISBLK
#undef S_ISCHR
#undef S_ISDIR
#undef S_ISREG
#undef S_ISFIFO
#undef S_ISLNK
#undef S_ISSOCK
#undef S_ISDOOR
#undef S_ISMPB
#undef S_ISMPC
#undef S_ISNWK
#endif

/* Do the reverse: define the POSIX.1 macros for traditional Unix systems
   that don't have them.  */
#if !defined(S_ISBLK) && defined(S_IFBLK)
#define	S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#endif
#if !defined(S_ISCHR) && defined(S_IFCHR)
#define	S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#endif
#if !defined(S_ISDIR) && defined(S_IFDIR)
#define	S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#if !defined(S_ISREG) && defined(S_IFREG)
#define	S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISFIFO) && defined(S_IFIFO)
#define	S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif
#if !defined(S_ISLNK) && defined(S_IFLNK)
#define	S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#endif
#if !defined(S_ISSOCK) && defined(S_IFSOCK)
#define	S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#if !defined(S_ISDOOR) && defined(S_IFDOOR)
#define	S_ISDOOR(m) (((m) & S_IFMT) == S_IFDOOR)
#endif
#if !defined(S_ISMPB) && defined(S_IFMPB) /* V7 */
/* Also available on Coherent, according to 
 * Albert D. Cahalan (acahalan@cs.uml.edu)
 */
#define S_ISMPB(m) (((m) & S_IFMT) == S_IFMPB) /* multiplexed block device */
#define S_ISMPC(m) (((m) & S_IFMT) == S_IFMPC) /* multiplexed char  device */
/* GNU BFD library source uses type letter 'm' for these */
#endif

#if !defined(S_ISNWK) && defined(S_IFNWK) /* HP/UX */
/* Apparently HPUX ls gives 'n' as the type letter for these. */
#define S_ISNWK(m) (((m) & S_IFMT) == S_IFNWK)
#endif

#endif

/* The above macros don't handle 
 * /bin/ls letters     Mode    What is it?
 *                     S_IFNAM (Xenix "name files")
 *  H                  S_ISCDF (HPUX Context Dependent Files)
 *                     S_IFCMP
 *                     S_IFSHAD
 */

/*
In message <199907051927.PAA01106@jupiter.cs.uml.edu>
Albert Cahalan wrote:-

BTW, I believe many of these can't actually exist on disk.
Some of these (like S_IFSHAD AFAIK) are not seen by userspace.

hex  name     ls octal  description
0000             000000 SCO out-of-service inode, BSD unknown type
1000 S_IFIFO  p| 010000 fifo (named pipe)
2000 S_IFCHR  c  020000 character special
3000 S_IFMPC     030000 multiplexed character device (Coherent)
4000 S_IFDIR  d/ 040000 directory
5000 S_IFNAM     050000 XENIX special named file
6000 S_IFBLK  b  060000 block special
7000 S_IFMPB     070000 multiplexed block device (Coherent)
8000 S_IFREG  -  100000 regular
9000 S_IFCMP     110000 VxFS compressed (file?)
9000 S_IFNWK     110000 HP-UX network special
a000 S_IFLNK  l@ 120000 symbolic link
b000 S_IFSHAD    130000 Solaris shadow inode for ACL
c000 S_IFSOCK s= 140000 socket (also "S_IFSOC" on VxFS)
d000 S_IFDOOR D  150000 Solaris door
e000 S_IFWHT  w% 160000 BSD whiteout (not used for inode)
f000 S_IFMT      170000 mask (not used for inode)
hex  name     ls octal  description
0200 S_ISVTX     001000 save swapped text even after use
0400 S_ISGID     002000 set group ID on execution
0400 S_ENFMT     002000 SysV forced file locking (shared w/ S_ISGID)
0800 S_CDF       004000 HP-UX hidden directory
0800 S_ISUID     004000 set user ID on execution


*/
