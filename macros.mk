#
# NetLabel Makefile Configuration & Macros
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
# simple /bin/sh script to find the top of the tree
#

TOPDIR = $$( \
	ftd() { \
		cd $$1; \
		if [ -r "macros.mk" ]; then \
			pwd; \
		else \
			ftd "../"; \
		 fi \
	}; \
	ftd .)

#
# build configuration
#

INCFLAGS = -I$(TOPDIR)/include
LIBFLAGS =

CFLAGS  = -O0 -g -Wall
LDFLAGS = -g

#
# build constants
#

VERSION_HDR = include/version.h

#
# build macros
#

ARCHIVE   = @echo " AR $@ (add/update: $?)"; $(AR) -cru $@ $?;
COMPILE   = @echo " CC $@"; $(CC) $(CFLAGS) $(INCFLAGS) -o $*.o -c $<;
LINK_EXEC = @echo " LD $@"; $(CC) $(LDFLAGS) -o $@ $^ $(LIBFLAGS);
LINK_LIB  = @echo " LD $@ "; $(CC) $(LDFLAGS) -o $@ $^ -shared -Wl,-soname=$@;

#
# default build targets
#

.c.o:
	$(COMPILE)



