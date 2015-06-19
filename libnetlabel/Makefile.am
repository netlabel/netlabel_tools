#
# NetLabel Library Makefile
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

SOURCES = \
	netlabel_comm.c netlabel_msg.c netlabel_init.c \
	mod_mgmt.c mod_unlabeled.c mod_cipsov4.c

noinst_LIBRARIES = libnetlabel.a

libnetlabel_a_SOURCES = ${SOURCES}
libnetlabel_a_CPPFLAGS = ${AM_CPPFLAGS} -I$(topdir)/include
libnetlabel_a_CFLAGS = ${AM_CFLAGS} ${CFLAGS} -fPIC -DPIC
