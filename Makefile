#
# NetLabel Tools Makefile
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

include macros.mk

#
# configuration
#

-include version_info.mk
-include configure.mk
include install.mk

#
# targets
#

CONFIGS = configure.mk configure.h version_info.mk version.h
SUBDIRS_BUILD = libnetlabel netlabelctl docs
SUBDIRS_INSTALL = netlabelctl docs

.PHONY: tarball install clean $(SUBDIRS_BUILD)

all: $(SUBDIRS_BUILD)

tarball: clean
	@ver=$$(source ./version_info; echo $$VERSION_RELEASE); \
	tarball=netlabel_tools-$$ver.tar.gz; \
	$(ECHO_INFO) "creating the tarball ../$$tarball"; \
	tmp_dir=$$(mktemp -d /tmp/netlabel_tools.XXXXX); \
	rel_dir=$$tmp_dir/netlabel_tools-$$ver; \
	$(MKDIR) $$rel_dir; \
	$(TAR) cf - --exclude=.svn . | (cd $$rel_dir; tar xf -); \
	(cd $$tmp_dir; tar zcf $$tarball netlabel_tools-$$ver); \
	mv $$tmp_dir/$$tarball ..; \
	rm -rf $$tmp_dir;

$(VERSION_HDR): version_info.mk
	@$(ECHO_INFO) "creating the version header file"
	@hdr="$(VERSION_HDR)"; \
	$(ECHO) "/* automatically generated - do not edit */" > $$hdr; \
	$(ECHO) "#ifndef _VERSION_H" >> $$hdr; \
	$(ECHO) "#define _VERSION_H" >> $$hdr; \
	$(ECHO) "#define VERSION_RELEASE \"$(VERSION_RELEASE)\"" >> $$hdr; \
	$(ECHO) "#define VERSION_MAJOR $(VERSION_MAJOR)" >> $$hdr; \
	$(ECHO) "#define VERSION_MINOR $(VERSION_MINOR)" >> $$hdr; \
	$(ECHO) "#endif" >> $$hdr;

libnetlabel: $(VERSION_HDR)
	@$(ECHO_INFO) "entering directory $@/ ..."
	@$(MAKE) -C $@

netlabelctl: $(VERSION_HDR) libnetlabel
	@$(ECHO_INFO) "entering directory $@/ ..."
	@$(MAKE) -C $@

docs: $(VERSION_HDR)
	@$(ECHO_INFO) "entering directory $@/ ..."
	@$(MAKE) -C $@

install: $(SUBDIRS_BUILD)
	@$(ECHO_INFO) "installing in $(INSTALL_PREFIX) ..."
	@for dir in $(SUBDIRS_INSTALL); do \
		$(ECHO_INFO) "installing from $$dir/"; \
		$(MAKE) -C $$dir install; \
	done

clean:
	@$(ECHO_INFO) "cleaning up"
	@for dir in $(SUBDIRS_BUILD); do \
		$(MAKE) -C $$dir clean; \
	done

dist-clean: clean
	@$(ECHO_INFO) "removing the configuration files"
	@$(RM) $(CONFIGS)
