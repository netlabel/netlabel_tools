/** @file
 * CIPSO/IPv4 Module Functions
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

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>

#include <libnetlabel.h>

#include "netlabel_internal.h"

/* Generic Netlink family ID */
static uint16_t nlbl_cipsov4_fid = 0;

/*
 * Helper functions
 */

/**
 * Create a new NetLabel CIPSOv4 message
 * @param command the NetLabel management command
 * @param flags the message flags
 *
 * This function creates a new NetLabel CIPSOv4 message using @command and
 * @flags.  Returns a pointer to the new message on success, or NULL on
 * failure.
 *
 */
static nlbl_msg *nlbl_cipsov4_msg_new(uint16_t command, int flags)
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
	nl_hdr->nlmsg_type = nlbl_cipsov4_fid;
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
 * Read a NetLbel CIPSOv4 message
 * @param hndl the NetLabel handle
 * @param msg the message
 *
 * Try to read a NetLabel CIPSOv4 message and return the message in @msg.
 * Returns the number of bytes read on success, zero on EOF, and negative
 * values on failure.
 *
 */
static int nlbl_cipsov4_recv(struct nlbl_handle *hndl, nlbl_msg **msg)
{
	int rc;
	struct nlmsghdr *nl_hdr;

	/* try to get a message from the handle */
	rc = nlbl_comm_recv(hndl, msg);
	if (rc <= 0)
		goto recv_failure;

	/* process the response */
	nl_hdr = nlbl_msg_nlhdr(*msg);
	if (nl_hdr == NULL || (nl_hdr->nlmsg_type != nlbl_cipsov4_fid &&
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
static int nlbl_cipsov4_parse_ack(nlbl_msg *msg)
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
 * Do any setup needed for the CIPSOv4 component, including determining the
 * NetLabel CIPSOv4 Generic Netlink family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_cipsov4_init(void)
{
	int rc = -ENOMEM;
	struct nlbl_handle *hndl;

	/* get a netlabel handle */
	hndl = nlbl_comm_open();
	if (hndl == NULL)
		goto init_return;

	/* resolve the family */
	rc = genl_ctrl_resolve(hndl->nl_sock, NETLBL_NLTYPE_CIPSOV4_NAME);
	if (rc < 0)
		goto init_return;
	nlbl_cipsov4_fid = rc;

	rc = 0;

init_return:
	nlbl_comm_close(hndl);
	return rc;
}

/*
 * NetLabel operations
 */

/**
 * Add a translated CIPSOv4 label mapping
 * @param hndl the NetLabel handle
 * @param doi the CIPSO DOI number
 * @param tags array of tags
 * @param lvls array of level mappings
 * @param cats array of category mappings, may be NULL
 *
 * Add the specified static CIPSO label mapping information to the NetLabel
 * system.  If @hndl is NULL then the function will handle opening and closing
 * it's own NetLabel handle.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_cipsov4_add_trans(struct nlbl_handle *hndl,
			   nlbl_cv4_doi doi,
			   struct nlbl_cv4_tag_a *tags,
			   struct nlbl_cv4_lvl_a *lvls,
			   struct nlbl_cv4_cat_a *cats)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *nest_msg_a = NULL;
	nlbl_msg *nest_msg_b = NULL;
	nlbl_msg *ans_msg = NULL;
	uint32_t iter;

	/* sanity checks */
	if (doi == 0 ||
	    tags == NULL || tags->size == 0 ||
	    lvls == NULL || lvls->size == 0)
		return -EINVAL;
	if (nlbl_cipsov4_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto add_std_return;
	}

	/* create a new message */
	msg = nlbl_cipsov4_msg_new(NLBL_CIPSOV4_C_ADD, 0);
	if (msg == NULL)
		goto add_std_return;

	/* add the required attributes to the message */

	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
	if (rc != 0)
		goto add_std_return;

	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_MTYPE, CIPSO_V4_MAP_TRANS);
	if (rc != 0)
		goto add_std_return;

	nest_msg_a = nlmsg_inherit(NULL);
	if (nest_msg_a == NULL) {
		rc = -ENOMEM;
		goto add_std_return;
	}
	for (iter = 0; iter < tags->size; iter++) {
		rc = nla_put_u8(nest_msg_a,
				NLBL_CIPSOV4_A_TAG, tags->array[iter]);
		if (rc != 0)
			goto add_std_return;
	}
	rc = nla_put_nested(msg, NLBL_CIPSOV4_A_TAGLST, nest_msg_a);
	if (rc != 0)
		goto add_std_return;
	nlbl_msg_free(nest_msg_a);
	nest_msg_a = NULL;

	nest_msg_a = nlmsg_inherit(NULL);
	if (nest_msg_a == NULL) {
		rc = -ENOMEM;
		goto add_std_return;
	}
	for (iter = 0; iter < lvls->size; iter++) {
		nest_msg_b = nlmsg_inherit(NULL);
		if (nest_msg_b == NULL) {
			rc = -ENOMEM;
			goto add_std_return;
		}
		rc = nla_put_u32(nest_msg_b,
				 NLBL_CIPSOV4_A_MLSLVLLOC,
				 lvls->array[iter * 2]);
		if (rc != 0)
			goto add_std_return;
		rc = nla_put_u32(nest_msg_b,
				 NLBL_CIPSOV4_A_MLSLVLREM,
				 lvls->array[iter * 2 + 1]);
		if (rc != 0)
			goto add_std_return;
		rc = nla_put_nested(nest_msg_a, NLBL_CIPSOV4_A_MLSLVL,
				    nest_msg_b);
		if (rc != 0)
			goto add_std_return;
		nlbl_msg_free(nest_msg_b);
		nest_msg_b = NULL;
	}
	rc = nla_put_nested(msg, NLBL_CIPSOV4_A_MLSLVLLST, nest_msg_a);
	if (rc != 0)
		goto add_std_return;
	nlbl_msg_free(nest_msg_a);
	nest_msg_a = NULL;

	nest_msg_a = nlmsg_inherit(NULL);
	if (nest_msg_a == NULL) {
		rc = -ENOMEM;
		goto add_std_return;
	}
	for (iter = 0; iter < cats->size; iter++) {
		nest_msg_b = nlmsg_inherit(NULL);
		if (nest_msg_b == NULL) {
			rc = -ENOMEM;
			goto add_std_return;
		}
		rc = nla_put_u32(nest_msg_b,
				 NLBL_CIPSOV4_A_MLSCATLOC,
				 cats->array[iter * 2]);
		if (rc != 0)
			goto add_std_return;
		rc = nla_put_u32(nest_msg_b,
				 NLBL_CIPSOV4_A_MLSCATREM,
				 cats->array[iter * 2 + 1]);
		if (rc != 0)
			goto add_std_return;
		rc = nla_put_nested(nest_msg_a, NLBL_CIPSOV4_A_MLSCAT,
				    nest_msg_b);
		if (rc != 0)
			goto add_std_return;
		nlbl_msg_free(nest_msg_b);
		nest_msg_b = NULL;
	}
	rc = nla_put_nested(msg, NLBL_CIPSOV4_A_MLSCATLST, nest_msg_a);
	if (rc != 0)
		goto add_std_return;
	nlbl_msg_free(nest_msg_a);
	nest_msg_a = NULL;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_std_return;
	}

	/* read the response */
	rc = nlbl_cipsov4_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_std_return;
	}

	/* process the response */
	rc = nlbl_cipsov4_parse_ack(ans_msg);

add_std_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(nest_msg_a);
	nlbl_msg_free(nest_msg_b);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Add a pass-through CIPSOv4 label mapping
 * @param hndl the NetLabel handle
 * @param doi the CIPSO DOI number
 * @param tags array of tags
 *
 * Add the specified static CIPSO label mapping information to the NetLabel
 * system.  If @hndl is NULL then the function will handle opening and closing
 * it's own NetLabel handle.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_cipsov4_add_pass(struct nlbl_handle *hndl,
			  nlbl_cv4_doi doi,
			  struct nlbl_cv4_tag_a *tags)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *nest_msg = NULL;
	nlbl_msg *ans_msg = NULL;
	uint32_t iter;

	/* sanity checks */
	if (doi == 0 ||
	    tags == NULL || tags->size == 0)
		return -EINVAL;
	if (nlbl_cipsov4_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto add_pass_return;
	}

	/* create a new message */
	msg = nlbl_cipsov4_msg_new(NLBL_CIPSOV4_C_ADD, 0);
	if (msg == NULL)
		goto add_pass_return;

	/* add the required attributes to the message */

	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
	if (rc != 0)
		goto add_pass_return;
	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_MTYPE, CIPSO_V4_MAP_PASS);
	if (rc != 0)
		goto add_pass_return;

	nest_msg = nlmsg_inherit(NULL);
	if (nest_msg == NULL) {
		rc = -ENOMEM;
		goto add_pass_return;
	}
	for (iter = 0; iter < tags->size; iter++) {
		rc = nla_put_u8(nest_msg,
				NLBL_CIPSOV4_A_TAG, tags->array[iter]);
		if (rc != 0)
			goto add_pass_return;
	}
	rc = nla_put_nested(msg, NLBL_CIPSOV4_A_TAGLST, nest_msg);
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
	rc = nlbl_cipsov4_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_pass_return;
	}

	/* process the response */
	rc = nlbl_cipsov4_parse_ack(ans_msg);

add_pass_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(nest_msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Add a local CIPSOv4 label mapping
 * @param hndl the NetLabel handle
 * @param doi the CIPSO DOI number
 *
 * Add the specified static CIPSO label mapping information to the NetLabel
 * system.  If @hndl is NULL then the function will handle opening and closing
 * it's own NetLabel handle.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_cipsov4_add_local(struct nlbl_handle *hndl, nlbl_cv4_doi doi)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *nest_msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (doi == 0)
		return -EINVAL;
	if (nlbl_cipsov4_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto add_local_return;
	}

	/* create a new message */
	msg = nlbl_cipsov4_msg_new(NLBL_CIPSOV4_C_ADD, 0);
	if (msg == NULL)
		goto add_local_return;

	/* add the required attributes to the message */

	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
	if (rc != 0)
		goto add_local_return;
	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_MTYPE, CIPSO_V4_MAP_LOCAL);
	if (rc != 0)
		goto add_local_return;

	nest_msg = nlmsg_inherit(NULL);
	if (nest_msg == NULL) {
		rc = -ENOMEM;
		goto add_local_return;
	}
	rc = nla_put_u8(nest_msg, NLBL_CIPSOV4_A_TAG, 128);
	if (rc != 0)
		goto add_local_return;
	rc = nla_put_nested(msg, NLBL_CIPSOV4_A_TAGLST, nest_msg);
	if (rc != 0)
		goto add_local_return;

	/* send the request */
	rc = nlbl_comm_send(p_hndl, msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_local_return;
	}

	/* read the response */
	rc = nlbl_cipsov4_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto add_local_return;
	}

	/* process the response */
	rc = nlbl_cipsov4_parse_ack(ans_msg);

add_local_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(nest_msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * Delete a CIPSOv4 label mapping
 * @param hndl the NetLabel handle
 * @param doi the CIPSO DOI number
 *
 * Remove the CIPSO label mapping with the DOI value matching @doi.  If @hndl
 * is NULL then the function will handle opening and closing it's own NetLabel
 * handle.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_cipsov4_del(struct nlbl_handle *hndl, nlbl_cv4_doi doi)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;

	/* sanity checks */
	if (doi == 0)
		return -EINVAL;
	if (nlbl_cipsov4_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto del_return;
	}

	/* create a new message */
	msg = nlbl_cipsov4_msg_new(NLBL_CIPSOV4_C_REMOVE, 0);
	if (msg == NULL)
		goto del_return;

	/* add the required attributes to the message */
	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
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
	rc = nlbl_cipsov4_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto del_return;
	}

	/* process the response */
	rc = nlbl_cipsov4_parse_ack(ans_msg);

del_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * List the details of a specific CIPSOv4 label mapping
 * @param hndl the NetLabel handle
 * @param doi the CIPSO DOI number
 * @param mtype the DOI mapping type
 * @param tags array of tag numbers
 * @param lvls array of level mappings
 * @param cats array of category mappings
 *
 * Query the kernel for the specified CIPSOv4 mapping specified by @doi and
 * return the details of the mapping to the caller.  If @hndl is NULL then the
 * function will handle opening and closing it's own NetLabel handle.  Returns
 * zero on success, negative values on failure.
 *
 */
int nlbl_cipsov4_list(struct nlbl_handle *hndl,
		      nlbl_cv4_doi doi,
		      nlbl_cv4_mtype *mtype,
		      struct nlbl_cv4_tag_a *tags,
		      struct nlbl_cv4_lvl_a *lvls,
		      struct nlbl_cv4_cat_a *cats)
{
	int rc = -ENOMEM;
	struct nlbl_handle *p_hndl = hndl;
	nlbl_msg *msg = NULL;
	nlbl_msg *ans_msg = NULL;
	struct genlmsghdr *genl_hdr;
	struct nlattr *nla_a;
	struct nlattr *nla_b;
	struct nlattr *nla_c;
	struct nlattr *nla_d;
	int nla_b_rem;

	/* sanity checks */
	if (doi == 0 ||
	    mtype == NULL || tags == NULL || lvls == NULL || cats == NULL)
		return -EINVAL;
	if (nlbl_cipsov4_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto list_return;
	}

	/* create a new message */
	msg = nlbl_cipsov4_msg_new(NLBL_CIPSOV4_C_LIST, 0);
	if (msg == NULL)
		goto list_return;

	/* add the required attributes to the message */
	rc = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
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
	rc = nlbl_cipsov4_recv(p_hndl, &ans_msg);
	if (rc <= 0) {
		if (rc == 0)
			rc = -ENODATA;
		goto list_return;
	}

	/* check the response */
	rc = nlbl_cipsov4_parse_ack(ans_msg);
	if (rc < 0 && rc != -ENOMSG)
		goto list_return;
	genl_hdr = nlbl_msg_genlhdr(ans_msg);
	if (genl_hdr == NULL || genl_hdr->cmd != NLBL_CIPSOV4_C_LIST) {
		rc = -EBADMSG;
		goto list_return;
	}

	/* process the response */

	lvls->size = 0;
	lvls->array = NULL;
	cats->size = 0;
	cats->array = NULL;

	nla_a = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MTYPE);
	if (nla_a == NULL)
		goto list_return;
	*mtype = nla_get_u32(nla_a);

	nla_a = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_TAGLST);
	if (nla_a == NULL)
		goto list_return;
	tags->size = 0;
	tags->array = NULL;
	nla_for_each_attr(nla_b, nla_data(nla_a), nla_len(nla_a), nla_b_rem)
	if (nla_b->nla_type == NLBL_CIPSOV4_A_TAG) {
		tags->array = realloc(tags->array, tags->size + 1);
		if (tags->array == NULL) {
			rc = -ENOMEM;
			goto list_return;
		}
		tags->array[tags->size++] = nla_get_u8(nla_b);
	}

	if (*mtype == CIPSO_V4_MAP_TRANS) {
		nla_a = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MLSLVLLST);
		if (nla_a == NULL)
			goto list_return;
		nla_for_each_attr(nla_b,
				  nla_data(nla_a), nla_len(nla_a), nla_b_rem)
		if (nla_b->nla_type == NLBL_CIPSOV4_A_MLSLVL) {
			lvls->array = realloc(lvls->array,
					      ((lvls->size + 1) * 2) *
					      sizeof(nlbl_cv4_lvl));
			if (lvls->array == NULL) {
				rc = -ENOMEM;
				goto list_return;
			}
			nla_c = nla_find(nla_data(nla_b),
					 nla_len(nla_b),
					 NLBL_CIPSOV4_A_MLSLVLLOC);
			if (nla_c == NULL)
				goto list_return;
			nla_d = nla_find(nla_data(nla_b),
					 nla_len(nla_b),
					 NLBL_CIPSOV4_A_MLSLVLREM);
			if (nla_d == NULL)
				goto list_return;
			lvls->array[lvls->size * 2] = nla_get_u32(nla_c);
			lvls->array[lvls->size * 2 + 1] = nla_get_u32(nla_d);
			lvls->size++;
		}

		nla_a = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MLSCATLST);
		if (nla_a == NULL)
			goto list_return;
		nla_for_each_attr(nla_b,
				  nla_data(nla_a), nla_len(nla_a), nla_b_rem)
		if (nla_b->nla_type == NLBL_CIPSOV4_A_MLSCAT) {
			cats->array = realloc(cats->array,
					      ((cats->size + 1) * 2) *
					      sizeof(nlbl_cv4_cat));
			if (cats->array == NULL) {
				rc = -ENOMEM;
				goto list_return;
			}
			nla_c = nla_find(nla_data(nla_b),
					 nla_len(nla_b),
					 NLBL_CIPSOV4_A_MLSCATLOC);
			if (nla_c == NULL)
				goto list_return;
			nla_d = nla_find(nla_data(nla_b),
					 nla_len(nla_b),
					 NLBL_CIPSOV4_A_MLSCATREM);
			if (nla_d == NULL)
				goto list_return;
			cats->array[cats->size * 2] = nla_get_u32(nla_c);
			cats->array[cats->size * 2 + 1] = nla_get_u32(nla_d);
			cats->size++;
		}
	}

	rc = 0;

list_return:
	if (hndl == NULL)
		nlbl_comm_close(p_hndl);
	nlbl_msg_free(msg);
	nlbl_msg_free(ans_msg);
	return rc;
}

/**
 * List the CIPSOv4 label mappings
 * @param hndl the NetLabel handle
 * @param dois an array of DOI values
 * @param mtypes an array of the mapping types
 *
 * Query the kernel for the configured CIPSOv4 mappings and return two arrays;
 * @dois which contains the DOI values and @mtypes which contains the
 * type of mapping.  If @hndl is NULL then the function will handle opening
 * and closing it's own NetLabel handle.  Returns the number of mappings on
 * success, zero if no mappings exist, and negative values on failure.
 *
 */
int nlbl_cipsov4_listall(struct nlbl_handle *hndl,
			 nlbl_cv4_doi **dois,
			 nlbl_cv4_mtype **mtypes)
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
	nlbl_cv4_doi *doi_a = NULL, *doi_a_new;
	nlbl_cv4_mtype *mtype_a = NULL, *mtype_a_new;
	uint32_t count = 0;

	/* sanity checks */
	if (dois == NULL || mtypes == NULL)
		return -EINVAL;
	if (nlbl_cipsov4_fid == 0)
		return -ENOPROTOOPT;

	/* open a handle if we need one */
	if (p_hndl == NULL) {
		p_hndl = nlbl_comm_open();
		if (p_hndl == NULL)
			goto listall_return;
	}

	/* create a new message */
	msg = nlbl_cipsov4_msg_new(NLBL_CIPSOV4_C_LISTALL, NLM_F_DUMP);
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
			    genl_hdr->cmd != NLBL_CIPSOV4_C_LISTALL) {
				rc = -EBADMSG;
				goto listall_return;
			}
			nla_head = (struct nlattr *)(&genl_hdr[1]);
			data_attrlen = genlmsg_attrlen(genl_hdr, 0);

			/* resize the arrays */
			doi_a_new = realloc(doi_a,
					    sizeof(nlbl_cv4_doi) * (count + 1));
			if (doi_a_new == NULL)
				goto listall_return;
			doi_a = doi_a_new;
			mtype_a_new = realloc(mtype_a,
					      sizeof(nlbl_cv4_mtype) * (count + 1));
			if (mtype_a_new == NULL)
				goto listall_return;
			mtype_a = mtype_a_new;

			/* get the attribute information */
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_CIPSOV4_A_DOI);
			if (nla == NULL)
				goto listall_return;
			doi_a[count] = nla_get_u32(nla);
			nla = nla_find(nla_head,
				       data_attrlen, NLBL_CIPSOV4_A_MTYPE);
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
