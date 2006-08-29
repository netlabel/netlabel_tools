/*
 * NetLabel Library Common Functions
 *
 * Author: Paul Moore <paul.moore@hp.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/netlabel.h>

#include <libnetlabel.h>

/*
 * NetLabel protocol functions
 */

/**
 * nlbl_common_ack_parse - Common ACK parser
 * @msg: the NetLabel message
 * @msg_len: the length in bytes
 * @ack_type: the type of ACK to check for
 *
 * Description:
 * Parse the ACK in @msg and return the negated return code, otherwise return
 * -ENOMSG.
 *
 */
int nlbl_common_ack_parse(nlbl_data *msg, 
			  ssize_t msg_len,
			  unsigned int ack_type)
{
  struct genlmsghdr *genl_hdr = (struct genlmsghdr *)msg;
  nlbl_data *msg_iter;

  if (genl_hdr->cmd != ack_type ||
      msg_len != GENL_HDRLEN + 2 * NETLBL_LEN_U32)
    return -ENOMSG;
  msg_iter = msg + GENL_HDRLEN + NETLBL_LEN_U32;

  return -nlbl_get_u32(msg_iter);
}
