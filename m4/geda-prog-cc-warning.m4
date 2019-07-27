# geda-prog-cc-warning.m4                               -*-Autoconf-*-
# serial 1.0

dnl Check whether the C compiler accepts a warning option
dnl Copyright (C) 2019  Roland Lutz
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

# Usage: AX_PROG_CC_WARNING([foo_bar], [foo-bar])
#
# Checks whether the C compiler accepts -Wfoo-bar and defines the
# following shell and output variables depending on the result:
#
#     Wfoo_bar_IF_SUPPORTED       = -Wfoo-bar       or empty
#     Wno_foo_bar_IF_SUPPORTED    = -Wno-foo-bar    or empty
#     Werror_foo_bar_IF_SUPPORTED = -Werror=foo-bar or empty

AC_DEFUN([AX_PROG_CC_WARNING], [
  AC_CACHE_CHECK([whether $CC accepts -W$2],
                 [ax_cv_prog_cc_W$1],
                 [ax_cv_prog_cc_W$1=no
                  ax_save_CFLAGS=$CFLAGS
                  CFLAGS="-W$2"
                  ax_save_c_werror_flag=$ac_c_werror_flag
                  ac_c_werror_flag=yes
                  AC_COMPILE_IFELSE([AC_LANG_PROGRAM()],
                                    [ax_cv_prog_cc_W$1=yes],
                                    [])
                  CFLAGS=$ax_save_CFLAGS
                  ac_c_werror_flag=$ax_save_c_werror_flag])

  if test "x$ax_cv_prog_cc_W$1" = xyes; then
    W$1_IF_SUPPORTED="-W$2"
    Wno_$1_IF_SUPPORTED="-Wno-$2"
    Werror_$1_IF_SUPPORTED="-Werror=$2"
  fi
  AC_SUBST([W$1_IF_SUPPORTED])
  AC_SUBST([Wno_$1_IF_SUPPORTED])
  AC_SUBST([Werror_$1_IF_SUPPORTED])
])
