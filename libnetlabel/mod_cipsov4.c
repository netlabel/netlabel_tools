/*
 * CIPSO/IPv4 Module Functions
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
			 nl_hdr->nlmsg_type != NLMSG_DONE)) {
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
  struct genlmsghdr *genl_hdr;
  struct nlattr *nla;

  genl_hdr = nlbl_msg_genlhdr(msg);
  if (genl_hdr == NULL || genl_hdr->cmd != NLBL_CIPSOV4_C_ACK)
    goto parse_ack_failure;
  nla = nlbl_attr_find(msg, NLBL_CIPSOV4_A_ERRNO);
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
  nlbl_msg *ans_msg = NULL;

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
  ret_val = nla_put(msg, NLBL_CIPSOV4_A_TAGLST, tags->size, tags->array);
  if (ret_val != 0)
    goto add_std_return;
  ret_val = nla_put(msg,
		    NLBL_CIPSOV4_A_MLSLVLLST,
		    lvls->size * sizeof(nlbl_cv4_lvl),
		    lvls->array);
  if (ret_val != 0)
    goto add_std_return;
  if (cats != NULL) {
    ret_val = nla_put(msg,
		      NLBL_CIPSOV4_A_MLSCATLST,
		      cats->size * sizeof(nlbl_cv4_cat),
		      cats->array);
    if (ret_val != 0)
      goto add_std_return;
  }

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
  nlbl_msg *ans_msg = NULL;

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
  ret_val = nla_put(msg, NLBL_CIPSOV4_A_TAGLST, tags->size, tags->array);
  if (ret_val != 0)
    goto add_pass_return;

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
  struct nlattr *nla;

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

  /* process the response */
  genl_hdr = nlbl_msg_genlhdr(ans_msg);
  if (genl_hdr == NULL || genl_hdr->cmd != NLBL_CIPSOV4_C_LIST)
    goto list_return;
  nla = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MTYPE);
  if (nla == NULL)
    goto list_return;
  *mtype = nla_get_u32(nla);
  nla = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_TAGLST);
  if (nla == NULL)
    goto list_return;
  tags->size = nla_len(nla);
  if (tags->size == 0)
    goto list_return;
  tags->array = malloc(tags->size);
  nla_memcpy(tags->array, nla, tags->size);
  nla = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MLSLVLLST);
  if (nla == NULL)
    goto list_return;
  lvls->size = nla_len(nla) / sizeof(nlbl_cv4_lvl);
  lvls->array = malloc(nla_len(nla));
  nla_memcpy(lvls->array, nla, lvls->size * sizeof(nlbl_cv4_lvl));
  nla = nlbl_attr_find(ans_msg, NLBL_CIPSOV4_A_MLSCATLST);
  if (nla == NULL)
    goto list_return;
  cats->size = nla_len(nla) / sizeof(nlbl_cv4_cat);
  cats->array = malloc(nla_len(nla));
  nla_memcpy(cats->array, nla, cats->size * sizeof(nlbl_cv4_cat));

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
  nlbl_msg *msg = NULL;
  nlbl_msg *ans_msg = NULL;
  struct nlmsghdr *nl_hdr;
  struct genlmsghdr *genl_hdr;
  struct nlattr *nla_head;
  struct nlattr *nla;
  int ans_msg_len;
  int ans_msg_attrlen;
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
    nlbl_msg_free(ans_msg);

    /* get the next set of messages */
    ret_val = nlbl_cipsov4_recv(p_hndl, &ans_msg);
    if (ret_val <= 0) {
      if (ret_val == 0)
	ret_val = -ENODATA;
      goto listall_return;
    }

    /* loop through the messages */
    ans_msg_len = ret_val;
    nl_hdr = nlbl_msg_nlhdr(ans_msg);
    while (nlmsg_ok(nl_hdr, ans_msg_len) && nl_hdr->nlmsg_type != NLMSG_DONE) {
      /* get the header pointers */
      genl_hdr = (struct genlmsghdr *)nlmsg_data(nl_hdr);
      if (genl_hdr == NULL || genl_hdr->cmd != NLBL_CIPSOV4_C_LISTALL)
	goto listall_return;
      nla_head = (struct nlattr *)(&genl_hdr[1]);
      ans_msg_attrlen = nlmsg_len(nl_hdr) - NLMSG_ALIGN(sizeof(*genl_hdr));

      /* resize the arrays */
      doi_a = realloc(doi_a, sizeof(nlbl_cv4_doi) * (count + 1));
      if (doi_a == NULL)
	goto listall_return;
      mtype_a = realloc(mtype_a, sizeof(nlbl_cv4_mtype) * (count + 1));
      if (mtype_a == NULL)
	goto listall_return;

      /* get the attribute information */
      nla = nla_find(nla_head, ans_msg_attrlen, NLBL_CIPSOV4_A_DOI);
      if (nla == NULL)
	goto listall_return;
      doi_a[count] = nla_get_u32(nla);
      nla = nla_find(nla_head, ans_msg_attrlen, NLBL_CIPSOV4_A_MTYPE);
      if (nla == NULL)
	goto listall_return;
      mtype_a[count] = nla_get_u32(nla);

      count++;

      /* next message */
      nl_hdr = nlmsg_next(nl_hdr, &ans_msg_len);
    }
  } while (ans_msg != NULL && nl_hdr->nlmsg_type != NLMSG_DONE);

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
  nlbl_msg_free(ans_msg);
  return ret_val;
}
