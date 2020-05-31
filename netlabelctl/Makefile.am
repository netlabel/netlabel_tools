#
# NetLabel Control Makefile
#
# Author: Paul Moore <paul@paul-moore.com>
#

#
# This program is free software: you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

EXTRA_DIST = netlabel.service

sbin_PROGRAMS = netlabelctl
dist_sbin_SCRIPTS = netlabel-config

dist_sysconf_DATA = netlabel.rules

if HAVE_SYSTEMD
systemdsystemunit_DATA = netlabel.service
udevrules_DATA = 90-netlabel.rules
EXTRA_DIST += $(udevrules_DATA)
endif

netlabelctl_SOURCES = \
	netlabelctl.h main.c mgmt.c map.c unlabeled.c cipso.c calipso.c
netlabelctl_CPPFLAGS = ${AM_CPPFLAGS} -I$(topdir)/include
netlabelctl_LDADD = ../libnetlabel/libnetlabel.a
