/*
 * NetLabel Message Functions
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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>

#include <libnetlabel.h>

#include "netlabel_internal.h"

/*
 * Allocation Functions
 */

/** 
 * nlb_new_msg - Create a new NetLabel message
 *
 * Description:
 * Creates a new NetLabel message and allocates space for both the Netlink and
 * Generic Netlink headers.
 *
 */
nlbl_msg *nlbl_msg_new(void)
{
  nlbl_msg *msg;
  struct genlmsghdr genl_hdr;

  /* create the message with a simple netlink header */
  msg = nlmsg_build_no_hdr();
  if (msg == NULL)
    goto msg_new_failure;

  /* add a generic netlink header */
  genl_hdr.cmd = 0;
  genl_hdr.version = NETLBL_PROTO_VERSION;
  genl_hdr.reserved = 0;
  if (nlmsg_append(msg, &genl_hdr, sizeof(genl_hdr), 1) != 0)
    goto msg_new_failure;

  return msg;

 msg_new_failure:
  if (msg)
    nlmsg_free(msg);
  return NULL;
}

/**
 * nlbl_msg_free - Free a NetLabel message
 * @msg: the NetLabel message
 *
 * Description:
 * Free the memory associated with a NetLabel message.
 *
 */
void nlbl_msg_free(nlbl_msg *msg)
{
  if (msg)
    return nlmsg_free(msg);
}

/*
 * Netlink Header Functions
 */

/**
 * nlbl_msg_nlhdr - Get the Netlink header from the NetLabel message
 * @msg: the NetLabel message
 *
 * Description:
 * Returns the Netlink header on success, or NULL on failure.
 *
 */
struct nlmsghdr *nlbl_msg_nlhdr(nlbl_msg *msg)
{
  if (msg != NULL)
    return nlmsg_hdr(msg);
  else
    return NULL;
}

/**
 * nlbl_msg_genlhdr - Get the Generic Netlink header from the NetLabel message
 * @msg: the NetLabel message
 *
 * Description:
 * Returns the Generic Netlink header on success, or NULL on failure.
 *
 */
struct genlmsghdr *nlbl_msg_genlhdr(nlbl_msg *msg)
{
  struct nlmsghdr *nl_hdr;

  if (msg == NULL)
    return NULL;

  nl_hdr = nlmsg_hdr(msg);
  if (nl_hdr == NULL)
    return NULL;

  return (struct genlmsghdr *)(&nl_hdr[1]);
}

/*
 * Netlink Message Functions
 */

/**
 * nlbl_msg_err - Return the nlmsgerr struct in the Netlink message
 * @msg: the NetLabel message
 *
 * Description:
 * Returns a pointer to the nlmsgerr struct in a NLMSG_ERROR Netlink message
 * or NULL on failure.
 *
 */
struct nlmsgerr *nlbl_msg_err(nlbl_msg *msg)
{
  struct nlmsghdr *nl_hdr;

  nl_hdr = nlbl_msg_nlhdr(msg);
  if (nl_hdr == NULL || nl_hdr->nlmsg_type != NLMSG_ERROR)
    return NULL;
  return nlmsg_data(nl_hdr);
}

/*
 * Netlink Attribute Functions
 */

/**
 * nlbl_attr_head - Get the head of the Netlink attributes from a NetLabel msg
 * @msg: the NetLabel message
 *
 * Description:
 * Returns the head of the Netlink attributes from a NetLabel message on
 * success, or NULL on failure.
 *
 */
struct nlattr *nlbl_attr_head(nlbl_msg *msg)
{
  struct genlmsghdr *genl_hdr;

  if (msg == NULL)
    return NULL;

  genl_hdr = nlbl_msg_genlhdr(msg);
  if (genl_hdr == NULL)
    return NULL;

  return (struct nlattr *)(&genl_hdr[1]);
}

/**
 * nlbl_attr_find - Find an attribute in a NetLabel message
 * @msg: the NetLabel message
 * @nla_type: the attribute type
 *
 * Description:
 * Search @msg looking for the @nla_type attribute.  Return the attribute on
 * success, or NULL on failure.
 *
 */
struct nlattr *nlbl_attr_find(nlbl_msg *msg, int nla_type)
{
  struct nlmsghdr *nl_hdr;
  struct nlattr *nla_head;
  size_t rem;

  if (msg == NULL)
    return NULL;

  nl_hdr = nlbl_msg_nlhdr(msg);
  if (nl_hdr == NULL)
    return NULL;

  nla_head = nlbl_attr_head(msg);
  if (nla_head == NULL)
    return NULL;

  rem = nlmsg_len(nl_hdr) - NLMSG_ALIGN(sizeof(struct genlmsghdr));
  
  return nla_find(nla_head, rem, nla_type);
}
