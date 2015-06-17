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

PROG_BUILD = netlabelctl
PROG_SCRIPT = netlabel-config
PROGS = $(PROG_BUILD) $(PROG_SCRIPT)

SYSD_UNIT = netlabel.service

CONF_FILE = netlabel.rules

OBJS = main.o mgmt.o map.o unlabeled.o cipsov4.o

LDFLAGS := ../libnetlabel/libnetlabel.a \
	`pkg-config --libs libnl-3.0` `pkg-config --libs libnl-genl-3.0` -lm

DEPS = $(OBJS:%.o=%.d)

#
# targets
#

.PHONY: all clean install install_progs install_sysd install_conf

all: $(PROG_BUILD)

install: install_prog install_sysd install_conf

-include $(DEPS)

$(PROG_BUILD): $(OBJS) ../libnetlabel/libnetlabel.a
	$(LINK_EXEC)

install_prog: $(PROGS)
	$(INSTALL_SBIN_MACRO)

install_sysd: $(SYSD_UNIT)
ifeq ($(CONF_SYSTEMD),1)
	$(INSTALL_SYSD_MACRO)
endif

install_conf: $(CONF_FILE)
	$(INSTALL_ETC_MACRO)

clean:
	$(RM) -f $(DEPS) $(OBJS) $(PROG_BUILD)
