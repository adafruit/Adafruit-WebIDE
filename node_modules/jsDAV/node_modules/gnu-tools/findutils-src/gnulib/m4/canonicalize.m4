#serial 12

# Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([AC_FUNC_CANONICALIZE_FILE_NAME],
  [
    AC_LIBOBJ([canonicalize])

    AC_CHECK_HEADERS_ONCE([sys/param.h])
    AC_CHECK_DECLS_ONCE([canonicalize_file_name])
    AC_CHECK_FUNCS_ONCE([canonicalize_file_name resolvepath])
  ])
