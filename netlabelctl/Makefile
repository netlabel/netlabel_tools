#
# NetLabel Control Makefile
#
# NetLabel Tools are a collection of user space programs and libraries for
# working with the Linux NetLabel subsystem.  The NetLabel subsystem manages
# static and dynamic label mappings for network protocols such as CIPSO and
# RIPSO.
#
# Author: Paul Moore <paul@paul-moore.com>
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

include $(TOPDIR)/version_info.mk
include $(TOPDIR)/configure.mk
include $(TOPDIR)/install.mk

PROG = netlabelctl

OBJS = main.o mgmt.o map.o unlabeled.o cipsov4.o

LDFLAGS := ../libnetlabel/libnetlabel.a -lnl -lm

DEPS = $(OBJS:%.o=%.d)

#
# targets
#

.PHONY: all clean

all: $(PROG)

-include $(DEPS)

$(PROG): $(OBJS) ../libnetlabel/libnetlabel.a
	$(LINK_EXEC)

install: $(PROG)
	$(INSTALL_SBIN_MACRO)

clean:
	$(RM) -f $(DEPS) $(OBJS) $(PROG)

