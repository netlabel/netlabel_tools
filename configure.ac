dnl #
dnl # NetLabel Tools
dnl #
dnl # Author: Paul Moore <paul@paul-moore.com>
dnl #

dnl #
dnl # This program is free software: you can redistribute it and/or modify
dnl # it under the terms of version 2 of the GNU General Public License as
dnl # published by the Free Software Foundation.
dnl #
dnl # This program is distributed in the hope that it will be useful,
dnl # but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl # GNU General Public License for more details.
dnl #
dnl # You should have received a copy of the GNU General Public License
dnl # along with this program.  If not, see <http://www.gnu.org/licenses/>.
dnl #

dnl ####
dnl netlabel_tools defines
dnl ####
AC_INIT([netlabel_tools], [0.0.0])

dnl ####
dnl autoconf configuration
dnl ####
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([configure.h])
AC_CONFIG_MACRO_DIR([m4])

dnl ####
dnl automake configuration
dnl ####
dnl NOTE: Automake < 1.12 didn't have serial-tests and gives an error if it
dnl sees this, but for automake >= 1.13 serial-tests is required so we have to
dnl include it.  Solution is to test for the version of automake (by running
dnl an external command) and provide it if necessary.  Note we have to do this
dnl entirely using m4 macros since automake queries this macro by running
dnl 'autoconf --trace ...'.
m4_define([serial_tests], [
    m4_esyscmd([automake --version |
                head -1 |
                awk '{split ($NF,a,"."); if (a[1] == 1 && a[2] >= 12) { print "serial-tests" }}'
    ])
])
dnl # NOTE: do not [quote] this parameter
AM_INIT_AUTOMAKE(-Wall foreign subdir-objects tar-pax serial_tests)

dnl ####
dnl build tools
dnl ####
AC_PROG_CC
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl ####
dnl pkg-config configuration
dnl ####
PKG_PROG_PKG_CONFIG

dnl ####
dnl libtool configuration
dnl ####
LT_INIT([shared pic-only])

dnl ####
dnl enable silent builds by default
dnl ####
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ####
dnl build flags
dnl ####
AM_CPPFLAGS="-I\${top_srcdir}/include"
AM_CFLAGS="-Wall"
AM_LDFLAGS="-Wl,-z -Wl,relro"
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

dnl ####
dnl version information
dnl ####
VERSION_MAJOR=$(echo ${VERSION} | cut -d'.' -f 1)
VERSION_MINOR=$(echo ${VERSION} | cut -d'.' -f 2)
VERSION_MICRO=$(echo ${VERSION} | cut -d'.' -f 3)
VERSION_STRING=$(echo ${VERSION})
AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_MICRO])
AC_SUBST([VERSION_STRING])

dnl ####
dnl libnl checks
dnl  -> http://www.infradead.org/~tgr/libnl
dnl ####
PKG_CHECK_MODULES(LIBNL3, libnl-3.0 >= 3.0, [have_libnl3=yes], [have_libnl3=no])
if (test "${have_libnl3}" = "yes"); then
	CFLAGS+=" $LIBNL3_CFLAGS"
	LIBS+=" $LIBNL3_LIBS"
fi
PKG_CHECK_MODULES(LIBNLGENL3, libnl-genl-3.0 >= 3.0, [have_libnlgenl3=yes], [have_libnlgenl3=no])
if (test "${have_libnlgenl3}" = "yes"); then
	CFLAGS+=" $LIBNLGENL3_CFLAGS"
	LIBS+=" $LIBNLGENL3_LIBS"
fi

dnl ####
dnl systemd checks
dnl  -> http://www.freedesktop.org/software/systemd/man/daemon.html
dnl ####
AC_ARG_WITH([systemdsystemunitdir],
	    [AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files])],,
	    [with_systemdsystemunitdir=auto])
AS_IF([test "x$with_systemdsystemunitdir" = "xyes" -o "x$with_systemdsystemunitdir" = "xauto"],
      [def_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)
       AS_IF([test "x$def_systemdsystemunitdir" = "x"],
       [AS_IF([test "x$with_systemdsystemunitdir" = "xyes"],
        [AC_MSG_ERROR([systemd support requested but pkg-config unable to query systemd package])])
        with_systemdsystemunitdir=no],
       [with_systemdsystemunitdir="$def_systemdsystemunitdir"])])
AS_IF([test "x$with_systemdsystemunitdir" != "xno"],
      [AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])])
AM_CONDITIONAL([HAVE_SYSTEMD], [test "x$with_systemdsystemunitdir" != "xno"])

dnl ####
dnl doxygen checks
dnl ####
AC_CHECK_PROG(have_doxygen, doxygen, "yes", "no")
AM_CONDITIONAL(DOXYGEN, test "$have_doxygen" = yes)

dnl ####
dnl coverity checks
dnl ####
AC_CHECK_PROG(have_coverity, cov-build, "yes", "no")
AM_CONDITIONAL(COVERITY, test "$have_coverity" = yes)

dnl ####
dnl makefiles
dnl ####
AC_CONFIG_FILES([
	Makefile
	libnetlabel/Makefile
	netlabelctl/Makefile
	doc/Makefile
])

dnl ####
dnl done
dnl ####
AC_OUTPUT
