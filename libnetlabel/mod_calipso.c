/** @file
 * CALIPSO Module Functions
 *
 * Author: Paul Moore <paul@paul-moore.com>
 * Author: Huw Davies <huw@codeweavers.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006
 * (c) Copyright Huw Davies <huw@codeweavers.com>, 2015
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
static uint16_t nlbl_calipso_fid = 0;

/*
 * Helper functions
 */

/**
 * Create a new NetLabel CALIPSO message
 * @param command the NetLabel management command
 * @param flags the message flags
 *
 * This function creates a new NetLabel CALIPSO message using @command and
 * @flags.  Returns a pointer to the new message on success, or NULL on
 * failure.
 *
 */
static nlbl_msg *nlbl_calipso_msg_new(uint16_t command, int flags)
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
	nl_hdr->nlmsg_type = nlbl_calipso_fid;
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
 * Read a NetLbel CALIPSO message
 * @param hndl the NetLabel handle
 * @param msg the message
 *
 * Try to read a NetLabel CALIPSO message and return the message in @msg.
 * Returns the number of bytes read on success, zero on EOF, and negative
 * values on failure.
 *
 */
static int nlbl_calipso_recv(struct nlbl_handle *hndl, nlbl_msg **msg)
{
	int rc;
	struct nlmsghdr *nl_hdr;

	/* try to get a message from the handle */
	rc = nlbl_comm_recv(hndl, msg);
	if (rc <= 0)
		goto recv_failure;

	/* process the response */
	nl_hdr = nlbl_msg_nlhdr(*msg);
	if (nl_hdr == NULL || (nl_hdr->nlmsg_type != nlbl_calipso_fid &&
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
static int nlbl_calipso_parse_ack(nlbl_msg *msg)
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
 * Perform any setup needed
 *
 * Do any setup needed for the CALIPSO component, including determining the
 * NetLabel CALIPSO Generic Netlink family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_calipso_init(void)
{
	int rc = -ENOMEM;
	struct nlbl_handle *hndl;

	/* get a netlabel handle */
	hndl = nlbl_comm_open();
	if (hndl == NULL)
		goto init_return;

	/* resolve the family */
	rc = genl_ctrl_resolve(hndl->nl_sock, NETLBL_NLTYPE_CALIPSO_NAME);
	if (rc < 0)
		goto init_return;
	nlbl_calipso_fid = rc;

	rc = 0;

init_return:
	nlbl_comm_close(hndl);
	return rc;
}

/*
 * NetLabel operations
 */

/**
 * Add a pass-through CALIPSO label mapping
 * @param hndl the NetLabel handle
 * @param doi the CALIPSO DOI number
 *
 * Add the specified static CALIPSO label mapping information to the NetLabel
 * system.  If @hndl is NULL then the function will handle opening and closing
 * it's own NetLabel handle.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_calipso_add_pass(struct nlbl_handle *hndl,
			  nlbl_clp_doi doi)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *nest_msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (doi == 0)
		return -EINVAL;
	if (nlbl_calipso_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto add_pass_return;
	}

	/* create a new message */
	msg = nlbl_calipso_msg_new(NLBL_CALIPSO_C_ADD, 0);
	if (msg == NULL)
		goto add_pass_return;

	/* add the required attributes to the message */

	rc = nla_put_u32(msg, NLBL_CALIPSO_A_DOI, doi);
	if (rc != 0)
		goto add_pass_return;
	rc = nla_put_u32(msg, NLBL_CALIPSO_A_MTYPE, CALIPSO_MAP_PASS);
	if (rc != 0)
		goto add_pass_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_pass_return;
	}

	/* read the response */
	rc = nlbl_calipso_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_pass_return;
	}

	/* process the response */
	rc = nlbl_calipso_parse_ack(ans_msg);

add_pass_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(nest_msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Delete a CALIPSO label mapping
 * @param hndl the NetLabel handle
 * @param doi the CALIPSO DOI number
 *
 * Remove the CALIPSO label mapping with the DOI value matching @doi.  If @hndl
 * is NULL then the function will handle opening and closing it's own NetLabel
 * handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_calipso_del(struct nlbl_handle *hndl, nlbl_clp_doi doi)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (doi == 0)
		return -EINVAL;
	if (nlbl_calipso_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto del_return;
	}

	/* create a new message */
	msg = nlbl_calipso_msg_new(NLBL_CALIPSO_C_REMOVE, 0);
	if (msg == NULL)
		goto del_return;

	/* add the required attributes to the message */
	rc = nla_put_u32(msg, NLBL_CALIPSO_A_DOI, doi);
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
	rc = nlbl_calipso_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto del_return;
	}

	/* process the response */
	rc = nlbl_calipso_parse_ack(ans_msg);

del_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * List the details of a specific CALIPSO label mapping
 * @param hndl the NetLabel handle
 * @param doi the CALIPSO DOI number
 * @param mtype the DOI mapping type
 *
 * Query the kernel for the specified CALIPSO mapping specified by @doi and
 * return the details of the mapping to the caller.  If @hndl is NULL then the
 * function will handle opening and closing it's own NetLabel handle.  Returns
 * zero on success, negative values on failure.
 *
 */
int nlbl_calipso_list(struct nlbl_handle *hndl,
		      nlbl_clp_doi doi,
		      nlbl_clp_mtype *mtype)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla_a;

	/* sanity checks */
	if (doi == 0 || mtype == NULL)
		return -EINVAL;
	if (nlbl_calipso_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto list_return;
	}

	/* create a new message */
	msg = nlbl_calipso_msg_new(NLBL_CALIPSO_C_LIST, 0);
	if (msg == NULL)
		goto list_return;

	/* add the required attributes to the message */
	rc = nla_put_u32(msg, NLBL_CALIPSO_A_DOI, doi);
	if (rc != 0)
		goto list_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto list_return;
	}

	/* read the response */
	rc = nlbl_calipso_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto list_return;
	}

	/* check the response */
	rc = nlbl_calipso_parse_ack(ans_msg);
	if (rc < 0 && rc != -ENOMSG)
		goto list_return;
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL || genl_hdr->cmd != NLBL_CALIPSO_C_LIST) {
		rc = -EBADMSG;
		goto list_return;
	}

	/* process the response */

	nla_a = nlbl_attr_find(ans_msg, NLBL_CALIPSO_A_MTYPE);
	if (nla_a == NULL)
		goto list_return;
	*mtype = nla_get_u32(nla_a);
	rc = 0;

list_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * List the CALIPSO label mappings
 * @param hndl the NetLabel handle
 * @param dois an array of DOI values
 * @param mtypes an array of the mapping types
 *
 * Query the kernel for the configured CALIPSO mappings and return two arrays;
 * @dois which contains the DOI values and @mtypes which contains the
 * type of mapping.  If @hndl is NULL then the function will handle opening
 * and closing it's own NetLabel handle.  Returns the number of mappings on
 * success, zero if no mappings exist, and negative values on failure.
 *
 */
int nlbl_calipso_listall(struct nlbl_handle *hndl,
			 nlbl_clp_doi **dois,
			 nlbl_clp_mtype **mtypes)
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
	nlbl_clp_doi *doi_a = NULL, *doi_a_new;
	nlbl_clp_mtype *mtype_a = NULL, *mtype_a_new;
	uint32_t count = 0;

	/* sanity checks */
	if (dois == NULL || mtypes == NULL)
		return -EINVAL;
	if (nlbl_calipso_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto listall_return;
	}

	/* create a new message */
	msg = nlbl_calipso_msg_new(NLBL_CALIPSO_C_LISTALL, NLM_F_DUMP);
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
		if (data) {
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
			    genl_hdr->cmd != NLBL_CALIPSO_C_LISTALL) {
				rc = -EBADMSG;
				goto listall_return;
			}
			nla_head = (struct nlattr *)(&genl_hdr[1]);
			data_attrlen = genlmsg_attrlen(genl_hdr, 0);

			/* resize the arrays */
			doi_a_new = realloc(doi_a,
					    sizeof(nlbl_clp_doi) * (count + 1));
			if (doi_a_new == NULL)
				goto listall_return;
			doi_a = doi_a_new;
			mtype_a_new = realloc(mtype_a,
					      sizeof(nlbl_clp_mtype) * (count + 1));
			if (mtype_a_new == NULL)
				goto listall_return;
			mtype_a = mtype_a_new;

			/* get the attribute information */
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_CALIPSO_A_DOI);
			if (nla == NULL)
				goto listall_return;
			doi_a[count] = nla_get_u32(nla);
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_CALIPSO_A_MTYPE);
			if (nla == NULL)
				goto listall_return;
			mtype_a[count] = nla_get_u32(nla);

			count++;

			/* next message */
			nl_hdr = nlmsg_next(nl_hdr, &data_len);
		}
	} while (NL_MULTI_CONTINUE(nl_hdr));

	*dois = doi_a;
	*mtypes = mtype_a;
	rc = count;

listall_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	if (rc < 0) {
		if (doi_a != NULL)
			free(doi_a);
		if (mtype_a != NULL)
			free(mtype_a);
	}
	if (data != NULL)
		free(data);
	nlbl_msg_free(msg);
	return rc;
}
