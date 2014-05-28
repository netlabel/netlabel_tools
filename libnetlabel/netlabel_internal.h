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

#ifndef _NETLINK_COMM_H
#define _NETLINK_COMM_H_

#include <netlink/netlink.h>

#define nl_handle_alloc             nl_socket_alloc
#define nl_handle_destroy           nl_socket_free
#define nl_handle                   nl_sock
#define nlmsg_build(ptr)            nlmsg_inherit(ptr)
#define nl_set_passcred             nl_socket_set_passcred
#define nl_disable_sequence_check   nl_socket_disable_seq_check
#define nlmsg_len                   nlmsg_datalen

/* NetLabel communication handle */
struct nlbl_handle {
	struct nl_handle *nl_hndl;
};

#define NL_MULTI_CONTINUE(hdr) \
	(((hdr)->nlmsg_type == 0) || \
	 (((hdr)->nlmsg_flags & NLM_F_MULTI) && \
	  ((hdr)->nlmsg_type != NLMSG_DONE)))


#endif
