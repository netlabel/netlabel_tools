#
# NetLabel Library Makefile
#
# NetLabel Tools are a collection of user space programs and libraries for
# working with the Linux NetLabel subsystem.  The NetLabel subsystem manages
# static and dynamic label mappings for network protocols such as CIPSO and
# RIPSO.
#
# Author: Paul Moore <paul.moore@hp.com>
#

#
# (c) Copyright Hewlett-Packard Development Company, L.P., 2006
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

#
# macros
#

include ../macros.mk

#
# configuration
#

OBJS =	netlabel_comm.o netlabel_msg.o netlabel_init.o \
	mod_mgmt.o mod_unlabeled.o mod_cipsov4.o 

LIB = libnetlabel.a

#
# targets
#

.PHONY: clean

all: $(LIB)

$(LIB): $(OBJS)
	$(ARCHIVE)

clean:
	$(RM) -f $(OBJS) $(LIB)

