#
# NetLabel Control Makefile
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
# This program is free software;  you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY;  without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program;  if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

#
# macros
#

include ../macros.mk

#
# configuration
#

OBJS = main.o mgmt.o map.o unlabeled.o cipsov4.o

LIBFLAGS = $(TOPDIR)/libnetlabel/libnetlabel.a -Lnl

#
# targets
#

.PHONY: clean

all: netlabelctl

netlabelctl: $(OBJS)
	$(LINK)

clean:
	$(RM) -f $(OBJS)


