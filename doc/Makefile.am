#
# NetLabel Documentation Makefile
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

dist_man8_MANS = \
	man/man8/netlabel-config.8 \
	man/man8/netlabelctl.8

if DOXYGEN
all-local: doxygen

clean-local: clean-doxygen

doxygen: doxyfile
	$(AM_V_P) || echo -e "  DOXYGEN  $<"
	doxygen $<

clean-doxygen:
	${RM} -rf doxygen
endif