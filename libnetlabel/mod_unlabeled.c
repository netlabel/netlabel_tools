/*
 * Unlabeled Module Functions
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/netlabel.h>

#include <libnetlabel.h>

#include "fields.h"
#include "common.h"

/* Generic NETLINK family ID */
static nlbl_type nlbl_unlabeled_fid = -1;

/*
 * Init functions
 */


/**
 * nlbl_unlabeled_init - Perform any setup needed
 *
 * Description:
 * Do any setup needed for the unlabeled component, including determining the
 * NetLabel Unlabeled Generic NETLINK family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_unlabeled_init(void)
{
  int ret_val;
  nlbl_socket sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;
  struct nlattr *nla_hdr;
  nlbl_type nl_type;

  /* open a socket */
  ret_val = nlbl_netlink_open(&sock);
  if (ret_val < 0)
    return ret_val;

  /* allocate a buffer for the family id request */
  msg_len = GENL_HDRLEN + NLA_HDRLEN + 
    strlen(NETLBL_NLTYPE_UNLABELED_NAME) + 1;
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto init_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the genetlink header into the buffer */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  genl_hdr->cmd = CTRL_CMD_GETFAMILY;
  genl_hdr->version = 1;

  /* write the attribute into the buffer */
  msg_iter += GENL_HDRLEN;
  nla_hdr = (struct nlattr *)msg_iter;
  nla_hdr->nla_len = NLA_HDRLEN + strlen(NETLBL_NLTYPE_UNLABELED_NAME) + 1;
  nla_hdr->nla_type = CTRL_ATTR_FAMILY_NAME;
  msg_iter += NLA_HDRLEN;
  strcpy((char *)msg_iter, NETLBL_NLTYPE_UNLABELED_NAME);

  /* send the message */
  ret_val = nlbl_netlink_write(sock, GENL_ID_CTRL, msg, msg_len);
  if (ret_val < 0)
    goto init_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the response */
  ret_val = nlbl_netlink_read(sock, &nl_type, &msg, &msg_len);
  if (ret_val < 0)
    goto init_return;
  if (nl_type != GENL_ID_CTRL) {
    ret_val = -ENOMSG;
    goto init_return;
  }
  msg_iter = msg + NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *) msg_iter;
  if (genl_hdr->cmd != CTRL_CMD_NEWFAMILY) {
    ret_val = -ENOMSG;
    goto init_return;
  }
  msg_iter += GENL_HDRLEN;
  do {
    nla_hdr = (struct nlattr *)msg_iter;
    msg_iter += NLMSG_ALIGN(nla_hdr->nla_len);
  } while (msg_iter - msg < msg_len || 
	   nla_hdr->nla_type != CTRL_ATTR_FAMILY_ID);
  if (nla_hdr->nla_type != CTRL_ATTR_FAMILY_ID) {
    ret_val = -ENOMSG;
    goto init_return;
  }
  nlbl_unlabeled_fid = nlbl_get_u16((unsigned char *)nla_hdr);

  ret_val = 0;

 init_return:
  if (msg)
    free(msg);
  nlbl_netlink_close(sock);
  return ret_val;
}

/*
 * Low-level communications
 */


/**
 * nlbl_unlabeled_write - Send a NetLabel unlabeled message
 * @sock: the socket
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Write the message in @msg to the NetLabel socket @sock.  Returns zero on
 * success, negative values on failure.
 *
 */
int nlbl_unlabeled_write(nlbl_socket sock, nlbl_data *msg, size_t msg_len)
{
  return nlbl_netlink_write(sock, nlbl_unlabeled_fid, msg, msg_len);
}

/**
 * nlbl_unlabeled_read - Read a NetLbel unlabled message
 * @sock: the socket
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Try to read a NetLabel unlabeled message and return the message in @msg.
 * Returns negative values on failure.
 *
 */
int nlbl_unlabeled_read(nlbl_socket sock, nlbl_data **msg, ssize_t *msg_len)
{
  int ret_val;
  nlbl_type nl_type;

  ret_val = nlbl_netlink_read(sock, &nl_type, msg, msg_len);
  if (ret_val >= 0 && nl_type != nlbl_unlabeled_fid)
      return -ENOMSG;

  return ret_val;
}

/*
 * NetLabel operations
 */


/**
 * nlbl_unlabeled_accept - Set the unlabeled accept flag
 * @sock: the NetLabel socket
 * @allow_flag: the desired accept flag setting
 *
 * Description:
 * Set the unlabeled accept flag in the NetLabel system; if @allow_flag is
 * true then set the accept flag, otherwise clear the flag.  If @sock is zero
 * then the function will handle opening and closing it's own NETLINK socket.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlabeled_accept(nlbl_socket sock, unsigned int allow_flag)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;

  /* sanity checks */
  if (sock < 0)
    return -EINVAL;

  /* open a socket if we need one */
  if (sock == 0) {
    ret_val = nlbl_netlink_open(&local_sock);
    if (ret_val < 0)
      return ret_val;
  }

  /* allocate a buffer for the message */
  msg_len = GENL_HDRLEN + NETLBL_LEN_U32;
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto accept_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  nlbl_putinc_genlhdr(&msg_iter, NLBL_UNLABEL_C_ACCEPT);
  if (allow_flag)
    nlbl_putinc_u32(&msg_iter, 1, NULL);
  else
    nlbl_putinc_u32(&msg_iter, 0, NULL);
  
  /* send the request */
  ret_val = nlbl_unlabeled_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto accept_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_unlabeled_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto accept_return;

  /* parse the response */
  ret_val = nlbl_common_ack_parse(msg + NLMSG_LENGTH(0),
				  msg_len - NLMSG_LENGTH(0),
				  NLBL_UNLABEL_C_ACK);

 accept_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_unlabeled_list - Query the unlabeled accept flag
 * @sock: the NetLabel socket
 * @allow_flag: the current accept flag setting
 *
 * Description:
 * Query the unlabeled accept flag in the NetLabel system.  If @sock is zero
 * then the function will handle opening and closing it's own NETLINK socket.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlabeled_list(nlbl_socket sock, unsigned int *allow_flag)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;

  /* sanity checks */
  if (sock < 0 || allow_flag == NULL)
    return -EINVAL;

  /* open a socket if we need one */
  if (sock == 0) {
    ret_val = nlbl_netlink_open(&local_sock);
    if (ret_val < 0)
      return ret_val;
  }

  /* allocate a buffer for the message */
  msg_len = GENL_HDRLEN;
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto list_return;
  }
  memset(msg, 0, msg_len);

  /* write the message into the buffer */
  nlbl_put_genlhdr(msg, NLBL_UNLABEL_C_LIST);
  
  /* send the request */
  ret_val = nlbl_unlabeled_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto list_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_unlabeled_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto list_return;
  msg_iter = msg + NLMSG_LENGTH(0);
  msg_len -= NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  if (genl_hdr->cmd != NLBL_UNLABEL_C_LIST) {
    ret_val = -ENOMSG;
    goto list_return;
  }
  msg_iter += GENL_HDRLEN;
  msg_len -= GENL_HDRLEN;
  if (msg_len != NETLBL_LEN_U32) {
    ret_val = -ENOMSG;
    goto list_return;
  }
  *allow_flag = nlbl_get_u32(msg_iter);

  ret_val = 0;

 list_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}
