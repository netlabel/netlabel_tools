#
# NetLabel Tools Makefile
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

include macros.mk

#
# configuration
#

INSTALL_PREFIX = /usr/local

INSTALL_SBIN_DIR = $(INSTALL_PREFIX)/sbin
INSTALL_BIN_DIR = $(INSTALL_PREFIX)/bin
INSTALL_MAN_DIR = $(INSTALL_PREFIX)/share/man

OWNER = root
GROUP = root

#
# targets
#

SUBDIRS = libnetlabel netlabelctl

.PHONY: tarball install clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	@echo "INFO: entering directory $@/ ..."
	@$(MAKE) -s -C $@

tarball: clean
	@name=$$(grep "^Name:" netlabel_tools.spec | awk '{ print $$2 }'); \
	ver=$$(grep "^Version:" netlabel_tools.spec | awk '{ print $$2 }'); \
	tarball=$$name-$$ver.tar.gz; \
	echo "INFO: creating the tarball ../$$tarball"; \
	tmp_dir=$$(mktemp -d /tmp/netlabel_tools.XXXXX); \
	rel_dir=$$tmp_dir/$$name-$$ver; \
	mkdir $$rel_dir; \
	tar cf - . | (cd $$rel_dir; tar xf -); \
	(cd $$tmp_dir; tar zcf $$tarball $$name-$$ver); \
	mv $$tmp_dir/$$tarball ..; \
	rm -rf $$tmp_dir;

install: $(SUBDIRS)
	@echo "INFO: installing files in $(INSTALL_PREFIX)"
	@mkdir -p $(INSTALL_SBIN_DIR)
	@mkdir -p $(INSTALL_MAN_DIR)/man8
	@install -o $(OWNER) -g $(GROUP) -m 755 netlabelctl/netlabelctl \
	 $(INSTALL_SBIN_DIR)/netlabelctl
	@install -o $(OWNER) -g $(GROUP) -m 644 docs/man/netlabelctl.8 \
	 $(INSTALL_MAN_DIR)/man8

clean:
	@for dir in $(SUBDIRS); do \
		echo "INFO: cleaning in $$dir/"; \
		$(MAKE) -s -C $$dir clean; \
	done

