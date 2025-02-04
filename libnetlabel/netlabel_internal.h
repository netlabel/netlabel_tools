/*
 * NetLabel Library Internals
 *
 * Author: Paul Moore <paul@paul-moore.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _NETLINK_COMM_H_
#define _NETLINK_COMM_H_

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

/* NetLabel communication handle */
struct nlbl_handle {
	struct nl_sock *nl_sock;
};

#define NL_MULTI_CONTINUE(hdr) \
	(((hdr)->nlmsg_type == 0) || \
	 (((hdr)->nlmsg_flags & NLM_F_MULTI) && \
	  ((hdr)->nlmsg_type != NLMSG_DONE)))

#endif
