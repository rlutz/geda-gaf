# geda-fam.m4                                         -*-Autoconf-*-
# serial 1.0

dnl MIME & desktop icon directories, and MIME database update options
dnl Copyright (C) 2009  Peter Brett <peter@peter-b.co.uk>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

# Check whether we should use libfam, and if so if libraries are
# available.
AC_DEFUN([AX_OPTION_FAM],
[
  AC_PREREQ([2.69])dnl
  AC_MSG_CHECKING([whether to use libfam])

  # Check what the user wants
  AC_ARG_WITH([libfam],
[AS_HELP_STRING([--with-libfam@<:@=DIR@:>@],
               [use libfam/libgamin (search in DIR)])
AS_HELP_STRING([--without-libfam],
               [don't use libfam/libgamin])],
    [ if test "X$with_libfam" = "Xno"; then
        libfam_use=no
      else
        libfam_use=yes
        if test "X$with_libfam" != "Xyes"; then
          libfam_prefix=$with_libfam
        fi
      fi
      AC_MSG_RESULT([$libfam_use]) ],
    [ AC_MSG_RESULT([yes])
  ])

  # Check if libfam is actually available!
  if test "X$libfam_use" != "Xno"; then

    # If a prefix to search was specified, then add the appropriate
    # flags.
    if test "X$libfam_use" = "X"; then
      LIBFAM_LDFLAGS="-L$libfam_prefix/lib"
      LIBFAM_CFLAGS="-I$libfam_prefix/include"
    fi

    # Check that the library and header file are available. Save and
    # restore CPPFLAGS and LDFLAGS variables.
    save_CPPFLAGS="$CPPFLAGS"
    save_LDFLAS="$LDFLAGS"
    CPPFLAGS="$CPPFLAGS $LIBFAM_CFLAGS"
    LDFLAGS="$LDFLAGS $LIBFAM_LDFLAGS"
    HAVE_LIBFAM=yes
    AC_CHECK_LIB([fam], [FAMOpen2], [], [HAVE_LIBFAM=no])
    AC_CHECK_HEADER([fam.h], [], [HAVE_LIBFAM=no
    CPPFLAGS="$save_CPPDFLAGS"
    LDFLAGS="$save_LDFLAGS"

    LIBFAM_LDFLAGS="$LIBFAM_LDFLAGS -lfam"])

    # Only continue without libfam if --without-libfam was specified.
    if test "X$HAVE_LIBFAM" = "Xno"; then
      AC_MSG_ERROR([Neither libgamin nor libfam development files could
be found.  Please ensure that the development files for either libgamin
or libfam are installed.

If you want to continue without File Alteration Monitor support, use
the configuration option `--without-libfam'.])
    fi
  fi

  # If we don't have libfam, clear its flags variables
  if test "X$HAVE_LIBFAM" != "Xyes"; then
    LIBFAM_LDFLAGS=""
    LIBFAM_CFLAGS=""
  else
    AC_DEFINE([HAVE_LIBFAM], [1],
      [Define to 1 if libfam is available])
  fi

  AC_SUBST([LIBFAM_CPPFLAGS])
  AC_SUBST([LIBFAM_LDFLAGS])

])dnl AX_OPTION_FAM
