/** @file
 * Management Module Functions
 *
 * Author: Paul Moore <paul@paul-moore.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006, 2008
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
static uint16_t nlbl_mgmt_fid = 0;

/*
 * Helper functions
 */

/**
 * Create a new NetLabel management message
 * @param command the NetLabel management command
 * @param flags the message flags
 *
 * This function creates a new NetLabel management message using @command and
 * @flags.  Returns a pointer to the new message on success, or NULL on
 * failure.
 *
 */
static nlbl_msg *nlbl_mgmt_msg_new(uint16_t command, int flags)
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
	nl_hdr->nlmsg_type = nlbl_mgmt_fid;
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
 * Read a NetLabel management message
 * @param hndl the NetLabel handle
 * @param msg the message
 *
 * Try to read a NetLabel management message and return the message in @msg.
 * Returns the number of bytes read on success, zero on EOF, and negative
 * values on failure.
 *
 */
static int nlbl_mgmt_recv(struct nlbl_handle *hndl, nlbl_msg **msg)
{
	int rc;
	struct nlmsghdr *nl_hdr;

	/* try to get a message from the handle */
	rc = nlbl_comm_recv(hndl, msg);
	if (rc <= 0)
		goto recv_failure;

	/* process the response */
	nl_hdr = nlbl_msg_nlhdr(*msg);
	if (nl_hdr == NULL || (nl_hdr->nlmsg_type != nlbl_mgmt_fid &&
			       nl_hdr->nlmsg_type != NLMSG_DONE &&
			       nl_hdr->nlmsg_type != NLMSG_ERROR)) {
		rc = -EBADMSG;
		goto recv_failure;
	}

	return rc;

recv_failure:
	nlbl_msg_free(*msg);
	return rc;
}

/**
 * Parse an ACK message
 * @param msg the message
 *
 * Parse the ACK message in @msg and return the error code specified in the
 * ACK.
 *
 */
static int nlbl_mgmt_parse_ack(nlbl_msg *msg)
{
	struct nlmsgerr *nl_err;

	nl_err = nlbl_msg_err(msg);
	if (nl_err == NULL)
		return -ENOMSG;

	return nl_err->error;
}

/**
 * Parse a LIST message with address selectors
 * @param nla_head the NLBL_MGMT_A_SELECTORLIST attribute
 * @param domain the domain mapping entry
 *
 * Parse the NLBL_MGMT_A_SELECTORLIST attribute and populate @domain with
 * the information.  Returns zero on success, negative values on failure.
 *
 */
static int nlbl_mgmt_list_addr(const struct nlattr *nla_head,
			       struct nlbl_dommap *domain)
{
	struct nlbl_dommap_addr *addr_iter;
	struct nlattr *nla_a;
	struct nlattr *nla_b;
	int nla_a_rem;

	domain->proto_type = NETLBL_NLTYPE_ADDRSELECT;

	/* parse the attributes */
	nla_for_each_attr(nla_a,
			  nla_data(nla_head), nla_len(nla_head),
			  nla_a_rem)
	if (nla_a->nla_type == NLBL_MGMT_A_ADDRSELECTOR) {
		addr_iter = malloc(sizeof(*addr_iter));
		if (addr_iter == NULL)
			return -ENOMEM;
		memset(addr_iter, 0, sizeof(*addr_iter));
		if (domain->proto.addrsel != NULL) {
			struct nlbl_dommap_addr *iter;
			iter = domain->proto.addrsel;
			while (iter->next != NULL)
				iter = iter->next;
			iter->next = addr_iter;
		} else
			domain->proto.addrsel = addr_iter;

		nla_b = nla_find(nla_data(nla_a), nla_len(nla_a),
				 NLBL_MGMT_A_IPV4ADDR);
		if (nla_b != NULL) {
			if (nla_len(nla_b) != sizeof(struct in_addr))
				return -EINVAL;
			memcpy(&addr_iter->addr.addr.v4,
			       nla_data(nla_b), nla_len(nla_b));
			nla_b = nla_find(nla_data(nla_a),
					 nla_len(nla_a),
					 NLBL_MGMT_A_IPV4MASK);
			if (nla_b == NULL ||
			    nla_len(nla_b) != sizeof(struct in_addr))
				return -EINVAL;
			memcpy(&addr_iter->addr.mask.v4,
			       nla_data(nla_b), nla_len(nla_b));
			addr_iter->addr.type = AF_INET;
			nla_b = nla_find(nla_data(nla_a),
					 nla_len(nla_a),
					 NLBL_MGMT_A_PROTOCOL);
			if (nla_b == NULL)
				return -EINVAL;
			addr_iter->proto_type = nla_get_u32(nla_b);
			switch (addr_iter->proto_type) {
			case NETLBL_NLTYPE_CIPSOV4:
				nla_b = nla_find(nla_data(nla_a),
						 nla_len(nla_a),
						 NLBL_MGMT_A_CV4DOI);
				if (nla_b == NULL)
					return -EINVAL;
				addr_iter->proto.cv4_doi =
					nla_get_u32(nla_b);
				break;
			}
		} else if ((nla_b = nla_find(nla_data(nla_a),
					     nla_len(nla_a),
					     NLBL_MGMT_A_IPV6ADDR))) {
			if (nla_len(nla_b) != sizeof(struct in6_addr))
				return -EINVAL;
			memcpy(&addr_iter->addr.addr.v6,
			       nla_data(nla_b), nla_len(nla_b));
			nla_b = nla_find(nla_data(nla_a),
					 nla_len(nla_a),
					 NLBL_MGMT_A_IPV6MASK);
			if (nla_b == NULL ||
			    nla_len(nla_b) != sizeof(struct in6_addr))
				return -EINVAL;
			memcpy(&addr_iter->addr.mask.v6,
			       nla_data(nla_b), nla_len(nla_b));
			addr_iter->addr.type = AF_INET6;
			nla_b = nla_find(nla_data(nla_a),
					 nla_len(nla_a),
					 NLBL_MGMT_A_PROTOCOL);
			if (nla_b == NULL)
				return -EINVAL;
			addr_iter->proto_type = nla_get_u32(nla_b);
		} else
			return -EINVAL;
	}

	return 0;
}

/*
 * Init functions
 */

/**
 * Perform any setup needed
 *
 * Do any setup needed for the management component, including determining the
 * NetLabel Mangement Generic Netlink family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_mgmt_init(void)
{
	int rc = -ENOMEM;
	struct nlbl_handle *hndl;

	/* get a netlabel handle */
	hndl = nlbl_comm_open();
	if (hndl == NULL)
		goto init_return;

	/* resolve the family */
	rc = genl_ctrl_resolve(hndl->nl_sock, NETLBL_NLTYPE_MGMT_NAME);
	if (rc < 0)
		goto init_return;
	nlbl_mgmt_fid = rc;

	rc = 0;

init_return:
	nlbl_comm_close(hndl);
	return rc;
}

/*
 * NetLabel operations
 */

/**
 * Determine the supported list of NetLabel protocols
 * @param hndl the NetLabel handle
 * @param protocols protocol array
 *
 * Query the NetLabel subsystem and return the supported protocols in
 * @protocols.  If @hndl is NULL then the function will handle opening and
 * closing it's own NetLabel handle.  Returns the number of protocols on
 * success, zero if no protocols are supported, and negative values on failure.
 *
 */
int nlbl_mgmt_protocols(struct nlbl_handle *hndl, nlbl_proto **protocols)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	unsigned char *data = NULL;
	nlbl_msg *msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla_head;
	struct nlattr *nla;
	int data_len;
	int data_attrlen;
	nlbl_proto *protos = NULL, *protos_new;
	uint32_t protos_count = 0;

	/* sanity checks */
	if (protocols == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto protocols_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_PROTOCOLS, NLM_F_DUMP);
	if (msg == NULL) {
		rc = -ENOMEM;
		goto protocols_return;
	}

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto protocols_return;
	}

	/* read all of the messages (multi-message response) */
	do {
		if (data != NULL) {
			free(data);
			data = NULL;
		}

		/* get the next set of messages */
		rc = nlbl_comm_recv_raw(p_hndl, &data);
		if (rc <= 0) {
			if (rc == 0)
				rc = -ENODATA;
			goto protocols_return;
		}
		data_len = rc;
		nl_hdr = (struct nlmsghdr *)data;

		/* check to see if this is a netlink control message we don't
		 * care about */
		if (nl_hdr->nlmsg_type == NLMSG_NOOP ||
		    nl_hdr->nlmsg_type == NLMSG_ERROR ||
		    nl_hdr->nlmsg_type == NLMSG_OVERRUN) {
			rc = -EBADMSG;
			goto protocols_return;
		}

		/* loop through the messages */
		while (nlmsg_ok(nl_hdr, data_len) &&
		       nl_hdr->nlmsg_type != NLMSG_DONE) {
			/* get the header pointers */
			genl_hdr = (struct genlmsghdr *)nlmsg_data(nl_hdr);
			if (genl_hdr == NULL ||
			    genl_hdr->cmd != NLBL_MGMT_C_PROTOCOLS) {
				rc = -EBADMSG;
				goto protocols_return;
			}
			nla_head = (struct nlattr *)(&genl_hdr[1]);
			data_attrlen = genlmsg_attrlen(genl_hdr, 0);

			/* resize the array */
			protos_new = realloc(protos,
					     sizeof(nlbl_proto) *
					     (protos_count + 1));
			if (protos_new == NULL)
				goto protocols_return;
			protos = protos_new;

			/* get the attribute information */
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_MGMT_A_PROTOCOL);
			if (nla == NULL)
				goto protocols_return;
			protos[protos_count] = nla_get_u32(nla);
			protos_count++;

			/* next message */
			nl_hdr = nlmsg_next(nl_hdr, &data_len);
		}
	} while (NL_MULTI_CONTINUE(nl_hdr));

	*protocols = protos;
	rc = protos_count;

protocols_return:
	if (rc < 0 && protos)
		free(protos);
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	if (data != NULL)
		free(data);
	nlbl_msg_free(msg);
	return rc;
}

/**
 * Determine the kernel's NetLabel protocol version
 * @param hndl the NetLabel handle
 * @param version the protocol version
 *
 * Request the NetLabel protocol version from the kernel and return the result
 * to the caller.  If @hndl is NULL then the function will handle opening and
 * closing it's own NetLabel handle.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_mgmt_version(struct nlbl_handle *hndl, uint32_t *version)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla;

	/* sanity checks */
	if (version == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto version_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_VERSION, 0);
	if (msg == NULL)
		goto version_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc == 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto version_return;
	}

	/* read the response */
	rc = nlbl_mgmt_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto version_return;
	}

	/* check the response */
	rc = nlbl_mgmt_parse_ack(ans_msg);
	if (rc < 0 && rc != -ENOMSG)
		goto version_return;
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL || genl_hdr->cmd != NLBL_MGMT_C_VERSION) {
		rc = -EBADMSG;
		goto version_return;
	}

	/* process the response */
	nla = nlbl_attr_find(ans_msg, NLBL_MGMT_A_VERSION);
	if (nla == NULL) {
		rc = -EBADMSG;
		goto version_return;
	}
	*version = nla_get_u32(nla);

	rc = 0;

version_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Add a domain mapping to the NetLabel system
 * @param hndl the NetLabel handle
 * @param domain the NetLabel domain map
 * @param addr the network IP address
 *
 * Add the domain mapping in @domain to the NetLabel system.  If @hndl is NULL
 * then the function will handle opening and closing it's own NetLabel handle.
 * Returns zero on success, negative values on failure.
 *
 */
int nlbl_mgmt_add(struct nlbl_handle *hndl,
		  struct nlbl_dommap *domain,
		  struct nlbl_netaddr *addr)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (domain == NULL || domain->domain == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto add_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_ADD, 0);
	if (msg == NULL)
		goto add_return;

	/* add the required attributes to the message */
	rc = nla_put_string(msg, NLBL_MGMT_A_DOMAIN, domain->domain);
	if (rc != 0)
		goto add_return;
	rc = nla_put_u32(msg, NLBL_MGMT_A_PROTOCOL, domain->proto_type);
	if (rc != 0)
		goto add_return;
	switch (domain->proto_type) {
	case NETLBL_NLTYPE_CIPSOV4:
		rc = nla_put_u32(msg,
				 NLBL_MGMT_A_CV4DOI,
				 domain->proto.cv4_doi);
		if (rc != 0)
			goto add_return;
		break;
	}

	/* optional attributes */
	switch (addr->type) {
	case AF_INET:
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV4ADDR,
			     sizeof(struct in_addr),
			     &addr->addr.v4);
		if (rc != 0)
			goto add_return;
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV4MASK,
			     sizeof(struct in_addr),
			     &addr->mask.v4);
		if (rc != 0)
			goto add_return;
		break;
	case AF_INET6:
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV6ADDR,
			     sizeof(struct in6_addr),
			     &addr->addr.v6);
		if (rc != 0)
			goto add_return;
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV6MASK,
			     sizeof(struct in6_addr),
			     &addr->mask.v6);
		if (rc != 0)
			goto add_return;
		break;
	case 0:
		rc = 0;
		break;
	default:
		rc = -EINVAL;
		goto add_return;
	}

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_return;
	}

	/* read the response */
	rc = nlbl_mgmt_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_return;
	}

	/* process the response */
	rc = nlbl_mgmt_parse_ack(ans_msg);

add_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Add the default domain mapping to the NetLabel system
 * @param hndl the NetLabel handle
 * @param domain the NetLabel domain map
 * @param addr the network IP address
 *
 * Add the domain mapping in @domain to the NetLabel system as the default
 * mapping.  If @hndl is NULL then the function will handle opening and
 * closing it's own NetLabel handle.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_mgmt_adddef(struct nlbl_handle *hndl,
		     struct nlbl_dommap *domain,
		     struct nlbl_netaddr *addr)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (domain == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto adddef_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_ADDDEF, 0);
	if (msg == NULL)
		goto adddef_return;

	/* add the required attributes to the message */
	rc = nla_put_u32(msg, NLBL_MGMT_A_PROTOCOL, domain->proto_type);
	if (rc != 0)
		goto adddef_return;
	switch (domain->proto_type) {
	case NETLBL_NLTYPE_CIPSOV4:
		rc = nla_put_u32(msg,
				 NLBL_MGMT_A_CV4DOI,
				 domain->proto.cv4_doi);
		if (rc != 0)
			goto adddef_return;
		break;
	}

	/* optional attributes */
	switch (addr->type) {
	case AF_INET:
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV4ADDR,
			     sizeof(struct in_addr),
			     &addr->addr.v4);
		if (rc != 0)
			goto adddef_return;
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV4MASK,
			     sizeof(struct in_addr),
			     &addr->mask.v4);
		if (rc != 0)
			goto adddef_return;
		break;
	case AF_INET6:
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV6ADDR,
			     sizeof(struct in6_addr),
			     &addr->addr.v6);
		if (rc != 0)
			goto adddef_return;
		rc = nla_put(msg,
			     NLBL_MGMT_A_IPV6MASK,
			     sizeof(struct in6_addr),
			     &addr->mask.v6);
		if (rc != 0)
			goto adddef_return;
		break;
	case 0:
		rc = 0;
		break;
	default:
		rc = -EINVAL;
		goto adddef_return;
	}

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto adddef_return;
	}

	/* read the response */
	rc = nlbl_mgmt_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto adddef_return;
	}

	/* process the response */
	rc = nlbl_mgmt_parse_ack(ans_msg);

adddef_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Remove a domain mapping from the NetLabel system
 * @param hndl the NetLabel handle
 * @param domain the domain
 *
 * Remove the domain mapping specified by @domain from the NetLabel system.
 * If @hndl is NULL then the function will handle opening and closing it's own
 * NetLabel handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_mgmt_del(struct nlbl_handle *hndl, char *domain)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (domain == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto del_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_REMOVE, 0);
	if (msg == NULL)
		goto del_return;

	/* add the required attributes to the message */
	rc = nla_put_string(msg, NLBL_MGMT_A_DOMAIN, domain);
	if (rc != 0)
		goto del_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto del_return;
	}

	/* read the response */
	rc = nlbl_mgmt_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto del_return;
	}

	/* process the response */
	rc = nlbl_mgmt_parse_ack(ans_msg);

del_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Remove the default domain mapping from NetLabel
 * @param hndl the NetLabel handle
 *
 * Remove the default domain mapping from the NetLabel system.  If @hndl is
 * NULL then the function will handle opening and closing it's own NetLabel
 * handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_mgmt_deldef(struct nlbl_handle *hndl)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto deldef_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_REMOVEDEF, 0);
	if (msg == NULL)
		goto deldef_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto deldef_return;
	}

	/* read the response */
	rc = nlbl_mgmt_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto deldef_return;
	}

	/* process the response */
	rc = nlbl_mgmt_parse_ack(ans_msg);

deldef_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * List the default NetLabel domain mapping
 * @param hndl the NetLabel handle
 * @param domain the default domain map
 *
 * Query the NetLabel subsystem and return the default domain mapping in
 * @domain.  If @hndl is NULL then the function will handle opening and
 * closing it's own NetLabel handle.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_mgmt_listdef(struct nlbl_handle *hndl, struct nlbl_dommap *domain)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla;

	/* sanity checks */
	if (domain == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto listdef_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_LISTDEF, 0);
	if (msg == NULL)
		goto listdef_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto listdef_return;
	}

	/* read the response */
	rc = nlbl_mgmt_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto listdef_return;
	}

	/* check the response */
	rc = nlbl_mgmt_parse_ack(ans_msg);
	if (rc < 0 && rc != -ENOMSG)
		goto listdef_return;
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL || genl_hdr->cmd != NLBL_MGMT_C_LISTDEF) {
		rc = -EBADMSG;
		goto listdef_return;
	}

	/* process the response */
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL || genl_hdr->cmd != NLBL_MGMT_C_LISTDEF)
		goto listdef_return;
	nla = nlbl_attr_find(ans_msg, NLBL_MGMT_A_PROTOCOL);
	if (nla != NULL) {
		domain->proto_type = nla_get_u32(nla);
		switch (domain->proto_type) {
		case NETLBL_NLTYPE_CIPSOV4:
			nla = nlbl_attr_find(ans_msg, NLBL_MGMT_A_CV4DOI);
			if (nla == NULL)
				goto listdef_return;
			domain->proto.cv4_doi = nla_get_u32(nla);
			break;
		}
	} else if ((nla = nlbl_attr_find(ans_msg, NLBL_MGMT_A_SELECTORLIST))) {
		if (nlbl_mgmt_list_addr(nla, domain) != 0)
			goto listdef_return;
	} else
		goto listdef_return;

	rc = 0;

listdef_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * List all of the configured NetLabel domain mappings
 * @param hndl the NetLabel handle
 * @param domains domain mapping array
 *
 * Query the NetLabel subsystem and return the configured domain mappings in
 * @domains.  If @hndl is NULL then the function will handle opening and
 * closing it's own NetLabel handle.  Returns the number of domains on success,
 * zero if no domains are specified, and negative values on failure.
 *
 */
int nlbl_mgmt_listall(struct nlbl_handle *hndl, struct nlbl_dommap **domains)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	unsigned char *data = NULL;
	nlbl_msg *msg = NULL;
	struct nlmsghdr *nl_hdr;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla_head;
	struct nlattr *nla;
	int data_len;
	int data_attrlen;
	struct nlbl_dommap *dmns = NULL, *dmns_new;
	uint32_t dmns_count = 0;

	/* sanity checks */
	if (domains == NULL)
		return -EINVAL;
	if (nlbl_mgmt_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto listall_return;
	}

	/* create a new message */
	msg = nlbl_mgmt_msg_new(NLBL_MGMT_C_LISTALL, NLM_F_DUMP);
	if (msg == NULL) {
		rc = -ENOMEM;
		goto listall_return;
	}

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto listall_return;
	}

	/* read all of the messages (multi-message response) */
	do {
		if (data != NULL) {
			free(data);
			data = NULL;
		}

		/* get the next set of messages */
		rc = nlbl_comm_recv_raw(p_hndl, &data);
		if (rc <= 0) {
			if (rc == 0)
				rc = -ENODATA;
			goto listall_return;
		}
		data_len = rc;
		nl_hdr = (struct nlmsghdr *)data;

		/* check to see if this is a netlink control message we don't
		 * care about */
		if (nl_hdr->nlmsg_type == NLMSG_NOOP ||
		    nl_hdr->nlmsg_type == NLMSG_ERROR ||
		    nl_hdr->nlmsg_type == NLMSG_OVERRUN) {
			rc = -EBADMSG;
			goto listall_return;
		}

		/* loop through the messages */
		while (nlmsg_ok(nl_hdr, data_len) &&
		       nl_hdr->nlmsg_type != NLMSG_DONE) {
			/* get the header pointers */
			genl_hdr = (struct genlmsghdr *)nlmsg_data(nl_hdr);
			if (genl_hdr == NULL ||
			    genl_hdr->cmd != NLBL_MGMT_C_LISTALL) {
				rc = -EBADMSG;
				goto listall_return;
			}
			nla_head = (struct nlattr *)(&genl_hdr[1]);
			data_attrlen = genlmsg_attrlen(genl_hdr, 0);

			/* resize the array */
			dmns_new = realloc(dmns,
					   sizeof(*dmns) * (dmns_count + 1));
			if (dmns_new == NULL)
				goto listall_return;
			dmns = dmns_new;
			memset(&dmns[dmns_count], 0, sizeof(*dmns));

			/* get the attribute information */
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_MGMT_A_DOMAIN);
			if (nla == NULL)
				goto listall_return;
			dmns[dmns_count].domain = malloc(nla_len(nla));
			if (dmns[dmns_count].domain == NULL)
				goto listall_return;
			strncpy(dmns[dmns_count].domain, nla_data(nla),
				nla_len(nla));
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_MGMT_A_PROTOCOL);
			if (nla != NULL) {
				dmns[dmns_count].proto_type = nla_get_u32(nla);
				switch (dmns[dmns_count].proto_type) {
				case NETLBL_NLTYPE_CIPSOV4:
					nla = nla_find(nla_head,
						       data_attrlen,
						       NLBL_MGMT_A_CV4DOI);
					if (nla == NULL)
						goto listall_return;
					dmns[dmns_count].proto.cv4_doi =
						nla_get_u32(nla);
					break;
				}
			} else if ((nla = nla_find(nla_head, data_attrlen,
						   NLBL_MGMT_A_SELECTORLIST))) {
				if (nlbl_mgmt_list_addr(nla,
							&dmns[dmns_count]) != 0)
					goto listall_return;
			} else
				goto listall_return;

			dmns_count++;

			/* next message */
			nl_hdr = nlmsg_next(nl_hdr, &data_len);
		}
	} while (NL_MULTI_CONTINUE(nl_hdr));

	*domains = dmns;
	rc = dmns_count;

listall_return:
	if (rc < 0 && dmns) {
		do {
			if (dmns[dmns_count].domain) {
				free(dmns[dmns_count].domain);
				if (dmns[dmns_count].proto_type ==
				    NETLBL_NLTYPE_ADDRSELECT) {
					struct nlbl_dommap_addr *iter, *prev;
					iter = dmns[dmns_count].proto.addrsel;
					while (iter) {
						prev = iter;
						iter = iter->next;
						free(prev);
					}
				}
			}
		} while (dmns_count-- > 0);
		free(dmns);
	}
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	if (data)
		free(data);
	nlbl_msg_free(msg);
	return rc;
}
