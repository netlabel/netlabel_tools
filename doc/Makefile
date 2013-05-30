#
# NetLabel Documentation Makefile
#
# NetLabel Tools are a collection of user space programs and libraries for
# working with the Linux NetLabel subsystem.  The NetLabel subsystem manages
# static and dynamic label mappings for network protocols such as CIPSO and
# RIPSO.
#
# Author: Paul Moore <paul@paul-moore.com>
#

#
# (c) Copyright Hewlett-Packard Development Company, L.P., 2008
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

DOXYGEN_CONF = doxyfile
DOXYGEN_OUTPUT_DIR = doxygen

MAN8 = \
	man/man8/netlabelctl.8 \
	man/man8/netlabel-config.8

#
# targets
#

.PHONY: all clean install install_man8

all: $(DOXYGEN_OUTPUT_DIR)

$(DOXYGEN_OUTPUT_DIR): $(DOXYGEN_CONF)
	@test -d $(DOXYGEN_OUTPUT_DIR) || mkdir $(DOXYGEN_OUTPUT_DIR);
	$(DOXYGEN_GEN)

install: install_man8

install_man8: $(MAN8)
	$(INSTALL_MAN8_MACRO)

clean:
	$(RM) -rf $(DOXYGEN_OUTPUT_DIR)

