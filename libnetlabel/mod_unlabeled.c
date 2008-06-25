/*
 * Unlabeled Module Functions
 *
 * Author: Paul Moore <paul.moore@hp.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006, 2007
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

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>

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
	if (nl_hdr == NULL || (nl_hdr->nlmsg_type != nlbl_unlbl_fid &&
			       nl_hdr->nlmsg_type != NLMSG_DONE &&
			       nl_hdr->nlmsg_type != NLMSG_ERROR)) {
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
	struct nlmsgerr *nl_err;

	nl_err = nlbl_msg_err(msg);
	if (nl_err == NULL)
		return -ENOMSG;

	return nl_err->error;
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
		goto init_return;

	/* create a new message */
	msg = nlbl_msg_new();
	if (msg == NULL)
		goto init_return;

	/* setup the netlink header */
	nl_hdr = nlbl_msg_nlhdr(msg);
	if (nl_hdr == NULL)
		goto init_return;
	nl_hdr->nlmsg_type = GENL_ID_CTRL;
  
	/* setup the generic netlink header */
	genl_hdr = nlbl_msg_genlhdr(msg);
	if (genl_hdr == NULL)
		goto init_return;
	genl_hdr->cmd = CTRL_CMD_GETFAMILY;
	genl_hdr->version = 1;

	/* add the netlabel family request attributes */
	ret_val = nla_put_string(msg,
				 CTRL_ATTR_FAMILY_NAME,
				 NETLBL_NLTYPE_UNLABELED_NAME);
	if (ret_val != 0)
		goto init_return;

	/* send the request */
	ret_val = nlbl_comm_send(hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto init_return;
	}

	/* read the response */
	ret_val = nlbl_comm_recv(hndl, &ans_msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto init_return;
	}
  
	/* process the response */
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL || genl_hdr->cmd != CTRL_CMD_NEWFAMILY) {
		ret_val = -EBADMSG;
		goto init_return;
	}
	nla = nlbl_attr_find(ans_msg, CTRL_ATTR_FAMILY_ID);
	if (nla == NULL) {
		ret_val = -EBADMSG;
		goto init_return;
	}
	nlbl_unlbl_fid = nla_get_u16(nla);
	if (nlbl_unlbl_fid == 0) {
		ret_val = -EBADMSG;
		goto init_return;
	}
  
	ret_val = 0;
  
init_return:
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
	ret_val = nlbl_comm_send(p_hndl, msg);
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
	ret_val = nlbl_comm_send(p_hndl, msg);
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

	/* check the response */
	ret_val = nlbl_unlbl_parse_ack(ans_msg);
	if (ret_val < 0 && ret_val != -ENOMSG)
		goto list_return;
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL) {
		ret_val = -EBADMSG;
		goto list_return;
	} else if (genl_hdr->cmd != NLBL_UNLABEL_C_LIST) {
		ret_val = -EBADMSG;
		goto list_return;
	}

	/* process the response */
	nla = nlbl_attr_find(ans_msg, NLBL_UNLABEL_A_ACPTFLG);
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

/**
 * nlbl_unlbl_staticadd - Add a static label configuration
 * @hndl: the NetLabel handle
 * @dev: the network interface
 * @addr: the network IP address
 * @label: the security label
 *
 * Description:
 * Add a new static label configuration to the NetLabel system.  If @hndl is
 * NULL then the function will handle opening and closing it's own NetLabel
 * handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_staticadd(nlbl_handle *hndl,
			 nlbl_netdev dev,
			 struct nlbl_netaddr *addr,
			 nlbl_secctx label)
{
	int ret_val = -ENOMEM;
	nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (dev == NULL || addr == NULL || label == NULL)
		return -EINVAL;
	if (nlbl_unlbl_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto staticadd_return;
	}

	/* create a new message */
	msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_STATICADD, 0);
	if (msg == NULL)
		goto staticadd_return;

	/* add the required attributes to the message */
	ret_val = nla_put_string(msg, NLBL_UNLABEL_A_IFACE, dev);
	if (ret_val != 0)
		goto staticadd_return;
	ret_val = nla_put_string(msg, NLBL_UNLABEL_A_SECCTX, label);
	if (ret_val != 0)
		goto staticadd_return;
	switch (addr->type) {
	case AF_INET:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4ADDR,
				  sizeof(struct in_addr),
				  &addr->addr.v4);
		if (ret_val != 0)
			goto staticadd_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4MASK,
				  sizeof(struct in_addr),
				  &addr->mask.v4);
		if (ret_val != 0)
			goto staticadd_return;
		break;
	case AF_INET6:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6ADDR,
				  sizeof(struct in6_addr),
				  &addr->addr.v6);
		if (ret_val != 0)
			goto staticadd_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6MASK,
				  sizeof(struct in6_addr),
				  &addr->mask.v6);
		if (ret_val != 0)
			goto staticadd_return;
		break;
	default:
		ret_val = -EINVAL;
		goto staticadd_return;
	}

	/* send the request */
	ret_val = nlbl_comm_send(p_hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticadd_return;
	}

	/* read the response */
	ret_val = nlbl_unlbl_recv(p_hndl, &ans_msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticadd_return;
	}

	/* process the response */
	ret_val = nlbl_unlbl_parse_ack(ans_msg);

staticadd_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return ret_val;
}

/**
 * nlbl_unlbl_staticadddef - Set the default static label configuration
 * @hndl: the NetLabel handle
 * @addr: the network IP address
 * @label: the security label
 *
 * Description:
 * Set the default static label configuration to the NetLabel system.  If
 * @hndl is NULL then the function will handle opening and closing it's own
 * NetLabel handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_staticadddef(nlbl_handle *hndl,
			    struct nlbl_netaddr *addr,
			    nlbl_secctx label)
{
	int ret_val = -ENOMEM;
	nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (addr == NULL || label == NULL)
		return -EINVAL;
	if (nlbl_unlbl_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto staticadddef_return;
	}

	/* create a new message */
	msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_STATICADDDEF, 0);
	if (msg == NULL)
		goto staticadddef_return;

	/* add the required attributes to the message */
	ret_val = nla_put_string(msg, NLBL_UNLABEL_A_SECCTX, label);
	if (ret_val != 0)
		goto staticadddef_return;
	switch (addr->type) {
	case AF_INET:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4ADDR,
				  sizeof(struct in_addr),
				  &addr->addr.v4);
		if (ret_val != 0)
			goto staticadddef_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4MASK,
				  sizeof(struct in_addr),
				  &addr->mask.v4);
		if (ret_val != 0)
			goto staticadddef_return;
		break;
	case AF_INET6:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6ADDR,
				  sizeof(struct in6_addr),
				  &addr->addr.v6);
		if (ret_val != 0)
			goto staticadddef_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6MASK,
				  sizeof(struct in6_addr),
				  &addr->mask.v6);
		if (ret_val != 0)
			goto staticadddef_return;
		break;
	default:
		ret_val = -EINVAL;
		goto staticadddef_return;
	}

	/* send the request */
	ret_val = nlbl_comm_send(p_hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticadddef_return;
	}

	/* read the response */
	ret_val = nlbl_unlbl_recv(p_hndl, &ans_msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticadddef_return;
	}

	/* process the response */
	ret_val = nlbl_unlbl_parse_ack(ans_msg);

staticadddef_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return ret_val;
}

/**
 * nlbl_unlbl_staticdel - Delete a static label configuration
 * @hndl: the NetLabel handle
 * @dev: the network interface
 * @addr: the network IP address
 *
 * Description:
 * Delete a new static label configuration to the NetLabel system.  If @hndl is
 * NULL then the function will handle opening and closing it's own NetLabel
 * handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_staticdel(nlbl_handle *hndl,
			 nlbl_netdev dev,
			 struct nlbl_netaddr *addr)
{
	int ret_val = -ENOMEM;
	nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (dev == NULL || addr == NULL)
		return -EINVAL;
	if (nlbl_unlbl_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto staticdel_return;
	}

	/* create a new message */
	msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_STATICREMOVE, 0);
	if (msg == NULL)
		goto staticdel_return;

	/* add the required attributes to the message */
	ret_val = nla_put_string(msg, NLBL_UNLABEL_A_IFACE, dev);
	if (ret_val != 0)
		goto staticdel_return;
	switch (addr->type) {
	case AF_INET:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4ADDR,
				  sizeof(struct in_addr),
				  &addr->addr.v4);
		if (ret_val != 0)
			goto staticdel_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4MASK,
				  sizeof(struct in_addr),
				  &addr->mask.v4);
		if (ret_val != 0)
			goto staticdel_return;
		break;
	case AF_INET6:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6ADDR,
				  sizeof(struct in6_addr),
				  &addr->addr.v6);
		if (ret_val != 0)
			goto staticdel_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6MASK,
				  sizeof(struct in6_addr),
				  &addr->mask.v6);
		if (ret_val != 0)
			goto staticdel_return;
		break;
	default:
		ret_val = -EINVAL;
		goto staticdel_return;
	}

	/* send the request */
	ret_val = nlbl_comm_send(p_hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticdel_return;
	}

	/* read the response */
	ret_val = nlbl_unlbl_recv(p_hndl, &ans_msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticdel_return;
	}

	/* process the response */
	ret_val = nlbl_unlbl_parse_ack(ans_msg);

staticdel_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return ret_val;
}

/**
 * nlbl_unlbl_staticdeldef - Delete the default static label configuration
 * @hndl: the NetLabel handle
 * @dev: the network interface
 * @addr: the network IP address
 *
 * Description:
 * Delete the default static label configuration to the NetLabel system.  If
 * @hndl is NULL then the function will handle opening and closing it's own
 * NetLabel handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_staticdeldef(nlbl_handle *hndl,
			    struct nlbl_netaddr *addr)
{
	int ret_val = -ENOMEM;
	nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (addr == NULL)
		return -EINVAL;
	if (nlbl_unlbl_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto staticdeldef_return;
	}

	/* create a new message */
	msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_STATICREMOVEDEF, 0);
	if (msg == NULL)
		goto staticdeldef_return;

	/* add the required attributes to the message */
	switch (addr->type) {
	case AF_INET:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4ADDR,
				  sizeof(struct in_addr),
				  &addr->addr.v4);
		if (ret_val != 0)
			goto staticdeldef_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV4MASK,
				  sizeof(struct in_addr),
				  &addr->mask.v4);
		if (ret_val != 0)
			goto staticdeldef_return;
		break;
	case AF_INET6:
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6ADDR,
				  sizeof(struct in6_addr),
				  &addr->addr.v6);
		if (ret_val != 0)
			goto staticdeldef_return;
		ret_val = nla_put(msg,
				  NLBL_UNLABEL_A_IPV6MASK,
				  sizeof(struct in6_addr),
				  &addr->mask.v6);
		if (ret_val != 0)
			goto staticdeldef_return;
		break;
	default:
		ret_val = -EINVAL;
		goto staticdeldef_return;
	}

	/* send the request */
	ret_val = nlbl_comm_send(p_hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticdeldef_return;
	}

	/* read the response */
	ret_val = nlbl_unlbl_recv(p_hndl, &ans_msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticdeldef_return;
	}

	/* process the response */
	ret_val = nlbl_unlbl_parse_ack(ans_msg);

staticdeldef_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return ret_val;
}

/**
 * nlbl_unlbl_staticlist - Dump the static label configuration
 * @hndl: the NetLabel handle
 * @addrs: the static label address mappings
 *
 * Description:
 * Dump the NetLabel static label configuration.  If @hndl is NULL then the
 * function will handle opening and closing it's own NetLabel handle.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_staticlist(nlbl_handle *hndl, struct nlbl_addrmap **addrs)
{
	int ret_val = -ENOMEM;
	nlbl_handle *p_hndl = hndl;
	unsigned char *data = NULL;
	nlbl_msg *msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla_head;
	struct nlattr *nla;
	int data_len;
	int data_attrlen;
	struct nlbl_addrmap *addr_array = NULL;
	uint32_t addr_count = 0;

	/* sanity checks */
	if (addrs == NULL)
		return -EINVAL;
	if (nlbl_unlbl_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto staticlist_return;
	}

	/* create a new message */
	msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_STATICLIST, NLM_F_DUMP);
	if (msg == NULL)
		goto staticlist_return;

	/* send the request */
	ret_val = nlbl_comm_send(p_hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticlist_return;
	}

	/* read all of the messages (multi-message response) */
	do {
		if (data) {
			free(data);
			data = NULL;
		}

		/* get the next set of messages */
		ret_val = nlbl_comm_recv_raw(p_hndl, &data);
		if (ret_val <= 0) {
			if (ret_val == 0)
				ret_val = -ENODATA;
			goto staticlist_return;
		}
		data_len = ret_val;
		nl_hdr = (struct nlmsghdr *)data;

		/* check to see if this is a netlink control message we don't care about */
		if (nl_hdr->nlmsg_type == NLMSG_NOOP ||
		    nl_hdr->nlmsg_type == NLMSG_ERROR ||
		    nl_hdr->nlmsg_type == NLMSG_OVERRUN) {
			ret_val = -EBADMSG;
			goto staticlist_return;
		}

		/* loop through the messages */
		while (nlmsg_ok(nl_hdr, data_len) && nl_hdr->nlmsg_type != NLMSG_DONE) {
			/* get the header pointers */
			genl_hdr = (struct genlmsghdr *)nlmsg_data(nl_hdr);
			if (genl_hdr == NULL || genl_hdr->cmd != NLBL_UNLABEL_C_STATICLIST) {
				ret_val = -EBADMSG;
				goto staticlist_return;
			}
			nla_head = (struct nlattr *)(&genl_hdr[1]);
			data_attrlen = nlmsg_len(nl_hdr) - NLMSG_ALIGN(sizeof(*genl_hdr));
      
			/* resize the array */
			addr_array = realloc(addr_array,
					     sizeof(struct nlbl_addrmap) * (addr_count + 1));
			if (addr_array == NULL)
				goto staticlist_return;
			memset(&addr_array[addr_count], 0, sizeof(struct nlbl_addrmap));

			/* get the attribute information */
			nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IFACE);
			if (nla == NULL)
				goto staticlist_return;
			addr_array[addr_count].dev = malloc(nla_len(nla));
			if (addr_array[addr_count].dev == NULL)
				goto staticlist_return;
			strncpy(addr_array[addr_count].dev, nla_data(nla), nla_len(nla));
			nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_SECCTX);
			if (nla == NULL)
				goto staticlist_return;
			addr_array[addr_count].label = malloc(nla_len(nla));
			if (addr_array[addr_count].label == NULL)
				goto staticlist_return;
			strncpy(addr_array[addr_count].label, nla_data(nla), nla_len(nla));
			if (nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV4ADDR)) {
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV4ADDR);
				if (nla == NULL)
					goto staticlist_return;
				if (nla_len(nla) != sizeof(struct in_addr))
					goto staticlist_return;
				memcpy(&addr_array[addr_count].addr.addr.v4,
				       nla_data(nla),
				       nla_len(nla));
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV4MASK);
				if (nla == NULL)
					goto staticlist_return;
				if (nla_len(nla) != sizeof(struct in_addr))
					goto staticlist_return;
				memcpy(&addr_array[addr_count].addr.mask.v4,
				       nla_data(nla),
				       nla_len(nla));
				addr_array[addr_count].addr.type = AF_INET;
			} else if (nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV6ADDR)) {
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV6ADDR);
				if (nla == NULL)
					goto staticlist_return;
				if (nla_len(nla) != sizeof(struct in6_addr))
					goto staticlist_return;
				memcpy(&addr_array[addr_count].addr.addr.v6,
				       nla_data(nla),
				       nla_len(nla));
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV6MASK);
				if (nla == NULL)
					goto staticlist_return;
				if (nla_len(nla) != sizeof(struct in6_addr))
					goto staticlist_return;
				memcpy(&addr_array[addr_count].addr.mask.v6,
				       nla_data(nla),
				       nla_len(nla));
				addr_array[addr_count].addr.type = AF_INET6;
			}
			addr_count++;
      
			/* next message */
			nl_hdr = nlmsg_next(nl_hdr, &data_len);
		}
	} while (data_len > 0 && nl_hdr->nlmsg_type != NLMSG_DONE);

	*addrs = addr_array;
	ret_val = addr_count;

staticlist_return:
	if (ret_val < 0 && addr_array) {
		do {
			if (addr_array[addr_count].dev)
				free(addr_array[addr_count].dev);
			if (addr_array[addr_count].label)
				free(addr_array[addr_count].label);
		} while (addr_count-- > 0);
		free(addr_array);
	}
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	return ret_val;
}

/**
 * nlbl_unlbl_staticlist - Dump the default static label configuration
 * @hndl: the NetLabel handle
 * @addrs: the static label address mappings
 *
 * Description:
 * Dump the NetLabel default static label configuration.  If @hndl is NULL
 * then the function will handle opening and closing it's own NetLabel handle.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_unlbl_staticlistdef(nlbl_handle *hndl, struct nlbl_addrmap **addrs)
{
	int ret_val = -ENOMEM;
	nlbl_handle *p_hndl = hndl;
	unsigned char *data = NULL;
	nlbl_msg *msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla_head;
	struct nlattr *nla;
	int data_len;
	int data_attrlen;
	struct nlbl_addrmap *addr_array = NULL;
	uint32_t addr_count = 0;

	/* sanity checks */
	if (addrs == NULL)
		return -EINVAL;
	if (nlbl_unlbl_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto staticlistdef_return;
	}

	/* create a new message */
	msg = nlbl_unlbl_msg_new(NLBL_UNLABEL_C_STATICLISTDEF, NLM_F_DUMP);
	if (msg == NULL)
		goto staticlistdef_return;

	/* send the request */
	ret_val = nlbl_comm_send(p_hndl, msg);
	if (ret_val <= 0) {
		if (ret_val == 0)
			ret_val = -ENODATA;
		goto staticlistdef_return;
	}

	/* read all of the messages (multi-message response) */
	do {
		if (data) {
			free(data);
			data = NULL;
		}

		/* get the next set of messages */
		ret_val = nlbl_comm_recv_raw(p_hndl, &data);
		if (ret_val <= 0) {
			if (ret_val == 0)
				ret_val = -ENODATA;
			goto staticlistdef_return;
		}
		data_len = ret_val;
		nl_hdr = (struct nlmsghdr *)data;

		/* check to see if this is a netlink control message we don't care about */
		if (nl_hdr->nlmsg_type == NLMSG_NOOP ||
		    nl_hdr->nlmsg_type == NLMSG_ERROR ||
		    nl_hdr->nlmsg_type == NLMSG_OVERRUN) {
			ret_val = -EBADMSG;
			goto staticlistdef_return;
		}

		/* loop through the messages */
		while (nlmsg_ok(nl_hdr, data_len) && nl_hdr->nlmsg_type != NLMSG_DONE) {
			/* get the header pointers */
			genl_hdr = (struct genlmsghdr *)nlmsg_data(nl_hdr);
			if (genl_hdr == NULL || genl_hdr->cmd != NLBL_UNLABEL_C_STATICLISTDEF) {
				ret_val = -EBADMSG;
				goto staticlistdef_return;
			}
			nla_head = (struct nlattr *)(&genl_hdr[1]);
			data_attrlen = nlmsg_len(nl_hdr) - NLMSG_ALIGN(sizeof(*genl_hdr));
      
			/* resize the array */
			addr_array = realloc(addr_array,
					     sizeof(struct nlbl_addrmap) * (addr_count + 1));
			if (addr_array == NULL)
				goto staticlistdef_return;
			memset(&addr_array[addr_count], 0, sizeof(struct nlbl_addrmap));

			/* get the attribute information */
			nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_SECCTX);
			if (nla == NULL)
				goto staticlistdef_return;
			addr_array[addr_count].label = malloc(nla_len(nla));
			if (addr_array[addr_count].label == NULL)
				goto staticlistdef_return;
			strncpy(addr_array[addr_count].label, nla_data(nla), nla_len(nla));
			if (nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV4ADDR)) {
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV4ADDR);
				if (nla == NULL)
					goto staticlistdef_return;
				if (nla_len(nla) != sizeof(struct in_addr))
					goto staticlistdef_return;
				memcpy(&addr_array[addr_count].addr.addr.v4,
				       nla_data(nla),
				       nla_len(nla));
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV4MASK);
				if (nla == NULL)
					goto staticlistdef_return;
				if (nla_len(nla) != sizeof(struct in_addr))
					goto staticlistdef_return;
				memcpy(&addr_array[addr_count].addr.mask.v4,
				       nla_data(nla),
				       nla_len(nla));
				addr_array[addr_count].addr.type = AF_INET;
			} else if (nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV6ADDR)) {
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV6ADDR);
				if (nla == NULL)
					goto staticlistdef_return;
				if (nla_len(nla) != sizeof(struct in6_addr))
					goto staticlistdef_return;
				memcpy(&addr_array[addr_count].addr.addr.v6,
				       nla_data(nla),
				       nla_len(nla));
				nla = nla_find(nla_head, data_attrlen, NLBL_UNLABEL_A_IPV6MASK);
				if (nla == NULL)
					goto staticlistdef_return;
				if (nla_len(nla) != sizeof(struct in6_addr))
					goto staticlistdef_return;
				memcpy(&addr_array[addr_count].addr.mask.v6,
				       nla_data(nla),
				       nla_len(nla));
				addr_array[addr_count].addr.type = AF_INET6;
			}
			addr_count++;
      
			/* next message */
			nl_hdr = nlmsg_next(nl_hdr, &data_len);
		}
	} while (data_len > 0 && nl_hdr->nlmsg_type != NLMSG_DONE);

	*addrs = addr_array;
	ret_val = addr_count;

staticlistdef_return:
	if (ret_val < 0 && addr_array) {
		do {
			if (addr_array[addr_count].label)
				free(addr_array[addr_count].label);
		} while (addr_count-- > 0);
		free(addr_array);
	}
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	return ret_val;
}
