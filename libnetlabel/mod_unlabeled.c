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
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <netlabel.h>
#include <libnetlabel.h>

#include "netlabel_internal.h"

/* Generic Netlink family ID */
static uint16_t nlbl_unlbl_fid = 0;

/*
 * Helper functions
 */

/**
 * nlbl_unlbl_msg_new - Create a new NetLabel unlbl message
 * @command: the NetLabel unlbl command
 *
 * Description:
 * This function creates a new NetLabel unlbl message using @command and
 * @flags.  Returns a pointer to the new message on success, or NULL on
 * failure.
 *
 */
static nlbl_msg *nlbl_unlbl_msg_new(uint16_t command, int flags)
{
  nlbl_msg *msg;
  struct nlmsghdr *nl_hdr;
  struct genlmsghdr *genl_hdr;

  /* create a new message */
  msg = nlbl_msg_new();
  if (msg == NULL)
    goto msg_new_failure;

  /* setup the netlink header */
  nl_hdr = nlbl_msg_nlhdr(msg);
  if (nl_hdr == NULL)
    goto msg_new_failure;
  nl_hdr->nlmsg_type = nlbl_unlbl_fid;
  nl_hdr->nlmsg_flags = flags;

  /* setup the generic netlink header */
  genl_hdr = nlbl_msg_genlhdr(msg);
  if (genl_hdr == NULL)
    goto msg_new_failure;
  genl_hdr->cmd = command;

  return msg;

 msg_new_failure:
  nlbl_msg_free(msg);
  return NULL;
}

/**
 * nlbl_unlbl_recv - Read a NetLbel unlbl message
 * @hndl: the NetLabel handle
 * @msg: the message
 *
 * Description:
 * Try to read a NetLabel unlbl message and return the message in @msg.
 * Returns the number of bytes read on success, zero on EOF, and negative
 * values on failure.
 *
 */
static int nlbl_unlbl_recv(nlbl_handle *hndl, nlbl_msg **msg)
{
  int ret_val;
  struct nlmsghdr *nl_hdr;

  /* try to get a message from the handle */
  ret_val = nlbl_comm_recv(hndl, msg);
  if (ret_val <= 0)
    goto recv_failure;
  
  /* process the response */
  nl_hdr = nlbl_msg_nlhdr(*msg);
  if (nl_hdr == NULL || nl_hdr->nlmsg_type != nlbl_unlbl_fid) {
    ret_val = -EBADMSG;
    goto recv_failure;
  }
  
  return ret_val;
  
 recv_failure:
  if (ret_val > 0)
    nlbl_msg_free(*msg);
  return ret_val;
}

/**
 * nlbl_unlbl_parse_ack - Parse an ACK message
 * @msg: the message
 *
 * Description:
 * Parse the ACK message in @msg and return the error code specified in the
 * ACK.
 *
 */
static int nlbl_unlbl_parse_ack(nlbl_msg *msg)
{
  struct genlmsghdr *genl_hdr;
  struct nlattr *nla;

  genl_hdr = nlbl_msg_genlhdr(msg);
  if (genl_hdr == NULL || genl_hdr->cmd != NLBL_UNLABEL_C_ACK)
    goto parse_ack_failure;
  nla = nlbl_attr_find(msg, NLBL_UNLABEL_A_ERRNO);
  if (nla == NULL)
    goto parse_ack_failure;

  return nla_get_u32(nla);

 parse_ack_failure:
  return -EBADMSG;
}

/*
 * Init functions
 */

/**
 * nlbl_unlbl_init - Perform any setup needed
 *
 * Description:
 * Do any setup needed for the unlbl component, including determining the
 * NetLabel unlbl Generic Netlink family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_unlbl_init(void)
{
  int ret_val = -ENOMEM;
  nlbl_handle *hndl;
  nlbl_msg *msg = NULL;
  nlbl_msg *ans_msg = NULL;
  struct nlmsghdr *nl_hdr;
  struct genlmsghdr *genl_hdr;
  struct nlattr *nla;

  /* get a netlabel handle */
  hndl = nlbl_comm_open();
  if (hndl == NULL)
    goto init_failure;

  /* create a new message */
  msg = nlbl_msg_new();
  if (msg == NULL)
    goto init_failure;

  /* setup the netlink header */
  nl_hdr = nlbl_msg_nlhdr(msg);
  if (nl_hdr == NULL)
    goto init_failure;
  nl_hdr->nlmsg_type = GENL_ID_CTRL;
  
  /* setup the generic netlink header */
  genl_hdr = nlbl_msg_genlhdr(msg);
  if (genl_hdr == NULL)
    goto init_failure;
  genl_hdr->cmd = CTRL_CMD_GETFAMILY;
  genl_hdr->version = 1;

  /* add the netlabel family request attributes */
  ret_val = nla_put_string(msg,
			   CTRL_ATTR_FAMILY_NAME,
			   NETLBL_NLTYPE_UNLABELED_NAME);
  if (ret_val != 0)
    goto init_failure;

  /* send the request */
  ret_val = nlbl_comm_send(hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto init_failure;
  }

  /* read the response */
  ret_val = nlbl_comm_recv(hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto init_failure;
  }
  
  /* process the response */
  genl_hdr = nlbl_msg_genlhdr(ans_msg);
  if (genl_hdr == NULL || genl_hdr->cmd != CTRL_CMD_NEWFAMILY) {
    ret_val = -EBADMSG;
    goto init_failure;
  }
  nla = nlbl_attr_find(ans_msg, CTRL_ATTR_FAMILY_ID);
  if (nla == NULL) {
    ret_val = -EBADMSG;
    goto init_failure;
  }
  nlbl_unlbl_fid = nla_get_u16(nla);
  if (nlbl_unlbl_fid == 0) {
    ret_val = -EBADMSG;
    goto init_failure;
  }
  
  return 0;
  
 init_failure:
  nlbl_comm_close(hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

/*
 * NetLabel operations
 */

/**
 * nlbl_unlbl_accept - Set the unlbl accept flag
 * @hndl: the NetLabel handle
 * @allow_flag: the desired accept flag setting
 *
 * Description:
 * Set the unlbl accept flag in the NetLabel system; if @allow_flag is
 * true then set the accept flag, otherwise clear the flag.  If @hndl is NULL
 * then the function will handle opening and closing it's own NetLabel handle.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_accept(nlbl_handle *hndl, uint8_t allow_flag)
{
  int ret_val = -ENOMEM;
  nlbl_handle *p_hndl = hndl;
  nlbl_msg *msg = NULL;
  nlbl_msg *ans_msg = NULL;

  /* sanity checks */
  if (nlbl_unlbl_fid == 0)
    return -ENOPROTOOPT;

  /* open a handle if we need one */
  if (p_hndl == NULL) {
    p_hndl = nlbl_comm_open();
    if (p_hndl == NULL)
      goto accept_return;
  }

  /* create a new message */
  msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_ACCEPT, 0);
  if (msg == NULL)
    goto accept_return;

  /* add the required attributes to the message */
  if (allow_flag)
    ret_val = nla_put_u8(msg, NLBL_UNLABEL_A_ACPTFLG, 1);
  else
    ret_val = nla_put_u8(msg, NLBL_UNLABEL_A_ACPTFLG, 0);
  if (ret_val != 0)
    goto accept_return;

  /* send the request */
  ret_val = nlbl_comm_send(hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto accept_return;
  }

  /* read the response */
  ret_val = nlbl_unlbl_recv(p_hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto accept_return;
  }

  /* process the response */
  ret_val = nlbl_unlbl_parse_ack(ans_msg);

 accept_return:
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

/**
 * nlbl_unlabeled_list - Query the unlbl accept flag
 * @hndl: the NetLabel handle
 * @allow_flag: the current accept flag setting
 *
 * Description:
 * Query the unlbl accept flag in the NetLabel system.  If @hndl is NULL then
 * the function will handle opening and closing it's own NetLabel handle.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_list(nlbl_handle *hndl, uint8_t *allow_flag)
{
  int ret_val = -ENOMEM;
  nlbl_handle *p_hndl = hndl;
  nlbl_msg *msg = NULL;
  nlbl_msg *ans_msg = NULL;
  struct genlmsghdr *genl_hdr;
  struct nlattr *nla;

  /* sanity checks */
  if (allow_flag == NULL)
    return -EINVAL;
  if (nlbl_unlbl_fid == 0)
    return -ENOPROTOOPT;

  /* open a handle if we need one */
  if (p_hndl == NULL) {
    p_hndl = nlbl_comm_open();
    if (p_hndl == NULL)
      goto list_return;
  }

  /* create a new message */
  msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_LIST, 0);
  if (msg == NULL)
    goto list_return;

  /* send the request */
  ret_val = nlbl_comm_send(hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto list_return;
  }

  /* read the response */
  ret_val = nlbl_unlbl_recv(p_hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto list_return;
  }

  /* process the response */
  genl_hdr = nlbl_msg_genlhdr(ans_msg);
  if (genl_hdr == NULL || genl_hdr->cmd != NLBL_UNLABEL_C_LIST)
    goto list_return;
  nla = nlbl_attr_find(msg, NLBL_UNLABEL_A_ACPTFLG);
  if (nla == NULL)
    goto list_return;
  *allow_flag = nla_get_u8(nla);

  ret_val = 0;

 list_return:
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

