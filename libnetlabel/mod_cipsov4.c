/*
 * CIPSO/IPv4 Module Functions
 *
 * Author: Paul Moore <paul.moore@hp.com>
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
 * nlbl_cipsov4_msg_new - Create a new NetLabel CIPSOv4 message
 * @command: the NetLabel management command
 *
 * Description:
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
 * nlbl_cipsov4_recv - Read a NetLbel CIPSOv4 message
 * @hndl: the NetLabel handle
 * @msg: the message
 *
 * Description:
 * Try to read a NetLabel CIPSOv4 message and return the message in @msg.
 * Returns the number of bytes read on success, zero on EOF, and negative
 * values on failure.
 *
 */
static int nlbl_cipsov4_recv(nlbl_handle *hndl, nlbl_msg **msg)
{
  int ret_val;
  struct nlmsghdr *nl_hdr;

  /* try to get a message from the handle */
  ret_val = nlbl_comm_recv(hndl, msg);
  if (ret_val <= 0)
    goto recv_failure;
  
  /* process the response */
  nl_hdr = nlbl_msg_nlhdr(*msg);
  if (nl_hdr == NULL || (nl_hdr->nlmsg_type != nlbl_cipsov4_fid &&
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
 * nlbl_cipsov4_parse_ack - Parse an ACK message
 * @msg: the message
 *
 * Description:
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
 * nlbl_cipsov4_init - Perform any setup needed
 *
 * Description:
 * Do any setup needed for the CIPSOv4 component, including determining the
 * NetLabel CIPSOv4 Generic Netlink family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_cipsov4_init(void)
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
			   NETLBL_NLTYPE_CIPSOV4_NAME);
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
  nlbl_cipsov4_fid = nla_get_u16(nla);
  if (nlbl_cipsov4_fid == 0) {
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
 * nlbl_cipsov4_add_std - Add a standard CIPSOv4 label mapping
 * @hndl: the NetLabel handle
 * @doi: the CIPSO DOI number
 * @tags: array of tags
 * @lvls: array of level mappings
 * @cats: array of category mappings, may be NULL
 * 
 * Description:
 * Add the specified static CIPSO label mapping information to the NetLabel
 * system.  If @hndl is NULL then the function will handle opening and closing
 * it's own NetLabel handle.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_cipsov4_add_std(nlbl_handle *hndl,
                         nlbl_cv4_doi doi,
                         nlbl_cv4_tag_a *tags,
                         nlbl_cv4_lvl_a *lvls,
                         nlbl_cv4_cat_a *cats)
{
  int ret_val = -ENOMEM;
  nlbl_handle *p_hndl = hndl;
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

  ret_val = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
  if (ret_val != 0)
    goto add_std_return;

  ret_val = nla_put_u32(msg, NLBL_CIPSOV4_A_MTYPE, CIPSO_V4_MAP_STD);
  if (ret_val != 0)
    goto add_std_return;

  nest_msg_a = nlmsg_build(NULL);
  if (nest_msg_a == NULL) {
    ret_val = -ENOMEM;
    goto add_std_return;
  }
  for (iter = 0; iter < tags->size; iter++) {
    ret_val = nla_put_u8(nest_msg_a, NLBL_CIPSOV4_A_TAG, tags->array[iter]);
    if (ret_val != 0)
      goto add_std_return;
  }
  nla_put_nested(msg, NLBL_CIPSOV4_A_TAGLST, nest_msg_a);
  nlbl_msg_free(nest_msg_a);
  nest_msg_a = NULL;

  nest_msg_a = nlmsg_build(NULL);
  if (nest_msg_a == NULL) {
    ret_val = -ENOMEM;
    goto add_std_return;
  }
  for (iter = 0; iter < lvls->size; iter++) {
    nest_msg_b = nlmsg_build(NULL);
    if (nest_msg_b == NULL) {
      ret_val = -ENOMEM;
      goto add_std_return;
    }
    ret_val = nla_put_u32(nest_msg_b,
			  NLBL_CIPSOV4_A_MLSLVLLOC,
			  lvls->array[iter * 2]);
    if (ret_val != 0)
      goto add_std_return;
    ret_val = nla_put_u32(nest_msg_b,
			  NLBL_CIPSOV4_A_MLSLVLREM,
			  lvls->array[iter * 2 + 1]);
    if (ret_val != 0)
      goto add_std_return;
    nla_put_nested(nest_msg_a, NLBL_CIPSOV4_A_MLSLVL, nest_msg_b);
    nlbl_msg_free(nest_msg_b);
    nest_msg_b = NULL;
  }
  nla_put_nested(msg, NLBL_CIPSOV4_A_MLSLVLLST, nest_msg_a);
  nlbl_msg_free(nest_msg_a);
  nest_msg_a = NULL;

  nest_msg_a = nlmsg_build(NULL);
  if (nest_msg_a == NULL) {
    ret_val = -ENOMEM;
    goto add_std_return;
  }
  for (iter = 0; iter < cats->size; iter++) {
    nest_msg_b = nlmsg_build(NULL);
    if (nest_msg_b == NULL) {
      ret_val = -ENOMEM;
      goto add_std_return;
    }
    ret_val = nla_put_u32(nest_msg_b,
			  NLBL_CIPSOV4_A_MLSCATLOC,
			  cats->array[iter * 2]);
    if (ret_val != 0)
      goto add_std_return;
    ret_val = nla_put_u32(nest_msg_b,
			  NLBL_CIPSOV4_A_MLSCATREM,
			  cats->array[iter * 2 + 1]);
    if (ret_val != 0)
      goto add_std_return;
    nla_put_nested(nest_msg_a, NLBL_CIPSOV4_A_MLSCAT, nest_msg_b);
    nlbl_msg_free(nest_msg_b);
    nest_msg_b = NULL;
  }
  nla_put_nested(msg, NLBL_CIPSOV4_A_MLSCATLST, nest_msg_a);
  nlbl_msg_free(nest_msg_a);
  nest_msg_a = NULL;

  /* send the request */
  ret_val = nlbl_comm_send(p_hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto add_std_return;
  }

  /* read the response */
  ret_val = nlbl_cipsov4_recv(p_hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto add_std_return;
  }

  /* process the response */
  ret_val = nlbl_cipsov4_parse_ack(ans_msg);

 add_std_return:
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(nest_msg_a);
  nlbl_msg_free(nest_msg_b);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

/**
 * nlbl_cipsov4_add_pass - Add a pass-through CIPSOv4 label mapping
 * @hndl: the NetLabel handle
 * @doi: the CIPSO DOI number
 * @tags: array of tags
 * 
 * Description:
 * Add the specified static CIPSO label mapping information to the NetLabel
 * system.  If @hndl is NULL then the function will handle opening and closing
 * it's own NetLabel handle.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_cipsov4_add_pass(nlbl_handle *hndl,
			  nlbl_cv4_doi doi,
			  nlbl_cv4_tag_a *tags)
{
  int ret_val = -ENOMEM;
  nlbl_handle *p_hndl = hndl;
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

  ret_val = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
  if (ret_val != 0)
    goto add_pass_return;
  ret_val = nla_put_u32(msg, NLBL_CIPSOV4_A_MTYPE, CIPSO_V4_MAP_PASS);
  if (ret_val != 0)
    goto add_pass_return;

  nest_msg = nlmsg_build(NULL);
  if (nest_msg == NULL) {
    ret_val = -ENOMEM;
    goto add_pass_return;
  }
  for (iter = 0; iter < tags->size; iter++) {
    ret_val = nla_put_u8(nest_msg, NLBL_CIPSOV4_A_TAG, tags->array[iter]);
    if (ret_val != 0)
      goto add_pass_return;
  }
  nla_put_nested(msg, NLBL_CIPSOV4_A_TAGLST, nest_msg);

  /* send the request */
  ret_val = nlbl_comm_send(p_hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto add_pass_return;
  }

  /* read the response */
  ret_val = nlbl_cipsov4_recv(p_hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto add_pass_return;
  }

  /* process the response */
  ret_val = nlbl_cipsov4_parse_ack(ans_msg);

 add_pass_return:
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(nest_msg);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

/**
 * nlbl_cipsov4_del - Delete a CIPSOv4 label mapping
 * @hndl: the NetLabel handle
 * @doi: the CIPSO DOI number
 *
 * Description:
 * Remove the CIPSO label mapping with the DOI value matching @doi.  If @hndl
 * is NULL then the function will handle opening and closing it's own NetLabel
 * handle.  Returns zero on success, negative values on failure. 
 *
 */
int nlbl_cipsov4_del(nlbl_handle *hndl, nlbl_cv4_doi doi)
{
  int ret_val = -ENOMEM;
  nlbl_handle *p_hndl = hndl;
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
  ret_val = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
  if (ret_val != 0)
    goto del_return;

  /* send the request */
  ret_val = nlbl_comm_send(p_hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto del_return;
  }

  /* read the response */
  ret_val = nlbl_cipsov4_recv(p_hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto del_return;
  }

  /* process the response */
  ret_val = nlbl_cipsov4_parse_ack(ans_msg);

 del_return:
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

/**
 * nlbl_cipsov4_list - List the details of a specific CIPSOv4 label mapping
 * @hndl: the NetLabel handle
 * @doi: the CIPSO DOI number
 * @mtype: the DOI mapping type
 * @tags: array of tag numbers
 * @lvls: array of level mappings
 * @cats: array of category mappings
 *
 * Description:
 * Query the kernel for the specified CIPSOv4 mapping specified by @doi and
 * return the details of the mapping to the caller.  If @hndl is NULL then the
 * function will handle opening and closing it's own NetLabel handle.  Returns
 * zero on success, negative values on failure.   
 *
 */
int nlbl_cipsov4_list(nlbl_handle *hndl,
                      nlbl_cv4_doi doi,
		      nlbl_cv4_mtype *mtype,
                      nlbl_cv4_tag_a *tags,
                      nlbl_cv4_lvl_a *lvls,
                      nlbl_cv4_cat_a *cats)
{
  int ret_val = -ENOMEM;
  nlbl_handle *p_hndl = hndl;
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
  ret_val = nla_put_u32(msg, NLBL_CIPSOV4_A_DOI, doi);
  if (ret_val != 0)
    goto list_return;

  /* send the request */
  ret_val = nlbl_comm_send(p_hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto list_return;
  }

  /* read the response */
  ret_val = nlbl_cipsov4_recv(p_hndl, &ans_msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto list_return;
  }

  /* check the response */
  ret_val = nlbl_cipsov4_parse_ack(ans_msg);
  if (ret_val < 0 && ret_val != -ENOMSG)
    goto list_return;
  genl_hdr = nlbl_msg_genlhdr(ans_msg);
  if (genl_hdr == NULL) {
    ret_val = -EBADMSG;
    goto list_return;
  } else if (genl_hdr->cmd != NLBL_CIPSOV4_C_LIST) {
    ret_val = -EBADMSG;
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
	ret_val = -ENOMEM;
	goto list_return;
      }
      tags->array[tags->size++] = nla_get_u8(nla_b);
    }

  if (*mtype == CIPSO_V4_MAP_STD) {
    nla_a = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MLSLVLLST);
    if (nla_a == NULL)
      goto list_return;
    nla_for_each_attr(nla_b, nla_data(nla_a), nla_len(nla_a), nla_b_rem)
      if (nla_b->nla_type == NLBL_CIPSOV4_A_MLSLVL) {
	lvls->array = realloc(lvls->array,
			      ((lvls->size + 1) * 2) * sizeof(nlbl_cv4_lvl));
	if (lvls->array == NULL) {
	  ret_val = -ENOMEM;
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
    nla_for_each_attr(nla_b, nla_data(nla_a), nla_len(nla_a), nla_b_rem)
      if (nla_b->nla_type == NLBL_CIPSOV4_A_MLSCAT) {
	cats->array = realloc(cats->array,
			      ((cats->size + 1) * 2) * sizeof(nlbl_cv4_cat));
	if (cats->array == NULL) {
	  ret_val = -ENOMEM;
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

  ret_val = 0;

 list_return:
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  nlbl_msg_free(ans_msg);
  return ret_val;
}

/**
 * nlbl_cipsov4_listall - List the CIPSOv4 label mappings
 * @hndl: the NetLabel handle
 * @dois: an array of DOI values
 * @mtypes: an array of the mapping types
 *
 * Description:
 * Query the kernel for the configured CIPSOv4 mappings and return two arrays;
 * @dois which contains the DOI values and @mtypes which contains the
 * type of mapping.  If @hndl is NULL then the function will handle opening
 * and closing it's own NetLabel handle.  Returns the number of mappings on
 * success, zero if no mappings exist, and negative values on failure.   
 *
 */
int nlbl_cipsov4_listall(nlbl_handle *hndl,
			 nlbl_cv4_doi **dois,
			 nlbl_cv4_mtype **mtypes)
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
  nlbl_cv4_doi *doi_a = NULL;
  nlbl_cv4_mtype *mtype_a = NULL;
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
    ret_val = -ENOMEM;
    goto listall_return;
  }

  /* send the request */
  ret_val = nlbl_comm_send(p_hndl, msg);
  if (ret_val <= 0) {
    if (ret_val == 0)
      ret_val = -ENODATA;
    goto listall_return;
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
      goto listall_return;
    }
    data_len = ret_val;
    nl_hdr = (struct nlmsghdr *)data;

    /* check to see if this is a netlink control message we don't care about */
    if (nl_hdr->nlmsg_type == NLMSG_NOOP ||
	nl_hdr->nlmsg_type == NLMSG_ERROR ||
	nl_hdr->nlmsg_type == NLMSG_OVERRUN) {
      ret_val = -EBADMSG;
      goto listall_return;
    }

    /* loop through the messages */
    while (nlmsg_ok(nl_hdr, data_len) && nl_hdr->nlmsg_type != NLMSG_DONE) {
      /* get the header pointers */
      genl_hdr = (struct genlmsghdr *)nlmsg_data(nl_hdr);
      if (genl_hdr == NULL || genl_hdr->cmd != NLBL_CIPSOV4_C_LISTALL) {
	ret_val = -EBADMSG;
	goto listall_return;
      }
      nla_head = (struct nlattr *)(&genl_hdr[1]);
      data_attrlen = nlmsg_len(nl_hdr) - NLMSG_ALIGN(sizeof(*genl_hdr));

      /* resize the arrays */
      doi_a = realloc(doi_a, sizeof(nlbl_cv4_doi) * (count + 1));
      if (doi_a == NULL)
	goto listall_return;
      mtype_a = realloc(mtype_a, sizeof(nlbl_cv4_mtype) * (count + 1));
      if (mtype_a == NULL)
	goto listall_return;

      /* get the attribute information */
      nla = nla_find(nla_head, data_attrlen, NLBL_CIPSOV4_A_DOI);
      if (nla == NULL)
	goto listall_return;
      doi_a[count] = nla_get_u32(nla);
      nla = nla_find(nla_head, data_attrlen, NLBL_CIPSOV4_A_MTYPE);
      if (nla == NULL)
	goto listall_return;
      mtype_a[count] = nla_get_u32(nla);

      count++;

      /* next message */
      nl_hdr = nlmsg_next(nl_hdr, &data_len);
    }
  } while (data > 0 && nl_hdr->nlmsg_type != NLMSG_DONE);

  *dois = doi_a;
  *mtypes = mtype_a;
  ret_val = count;

 listall_return:
  if (ret_val < 0) {
    if (doi_a)
      free(doi_a);
    if (mtype_a)
      free(mtype_a);
  }
  if (hndl == NULL)
    nlbl_comm_close(p_hndl);
  nlbl_msg_free(msg);
  if (data)
    free(data);
  return ret_val;
}
