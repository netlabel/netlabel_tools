#
# NetLabel Tools Makefile
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

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = libnetlabel netlabelctl tests doc

EXTRA_DIST = CHANGELOG LICENSE README

# support silent builds
AM_MAKEFLAGS_0 = --quiet --no-print-directory
AM_MAKEFLAGS_1 =
AM_MAKEFLAGS_ = ${AM_MAKEFLAGS_0}
AM_MAKEFLAGS = ${AM_MAKEFLAGS_@AM_V@}

# support systemd/distcheck
# -> http://www.freedesktop.org/software/systemd/man/daemon.html
DISTCHECK_CONFIGURE_FLAGS = \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

check-syntax:
	@./tools/check-syntax

if COVERITY
coverity-build: clean
	cov-build --dir cov-int ${MAKE} ${AM_MAKEFLAGS}
endif

if COVERITY
coverity-tarball: coverity-build
	@if git rev-parse HEAD &> /dev/null; then \
		rev_full=$$(git rev-parse HEAD); \
		rev=$$(echo $$rev_full | cut -c1-8); \
	else \
		rev_full=$$(date --iso-8601=date); \
		rev=$$rev_full; \
	fi; \
	tar czf netlabel_tools-coverity_$$rev.tar.gz cov-int; \
	echo " HEAD revision: $$rev_full"; \
	ls -l netlabel_tools-coverity_$$rev.tar.gz
endif

clean-local:
	${RM} -rf cov-int netlabel_tools-coverity_*.tar.gz
