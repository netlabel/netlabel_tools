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
static nlbl_type nlbl_cipsov4_fid = -1;

/*
 * Init functions
 */


/**
 * nlbl_cipsov4_init - Perform any setup needed
 *
 * Description:
 * Do any setup needed for the CIPSOv4 component, including determining the
 * NetLabel CIPSOv4 Generic NETLINK family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_cipsov4_init(void)
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
  msg_len = GENL_HDRLEN + NLA_HDRLEN + strlen(NETLBL_NLTYPE_CIPSOV4_NAME) + 1;
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
  nla_hdr->nla_len = NLA_HDRLEN + strlen(NETLBL_NLTYPE_CIPSOV4_NAME) + 1;
  nla_hdr->nla_type = CTRL_ATTR_FAMILY_NAME;
  msg_iter += NLA_HDRLEN;
  strcpy((char *)msg_iter, NETLBL_NLTYPE_CIPSOV4_NAME);

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
  nlbl_cipsov4_fid = nlbl_get_u16((unsigned char *)nla_hdr);

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
 * nlbl_cipsov4_write - Send a NetLabel CIPSOv4 message
 * @sock: the socket
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Write the message in @msg to the NetLabel socket @sock.  Returns zero on
 * success, negative values on failure.
 *
 */
int nlbl_cipsov4_write(nlbl_socket sock, nlbl_data *msg, size_t msg_len)
{
  return nlbl_netlink_write(sock, nlbl_cipsov4_fid, msg, msg_len);
}

/**
 * nlbl_cipsov4_read - Read a NetLbel CIPSOv4 message
 * @sock: the socket
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Try to read a NetLabel CIPSOv4 message and return the message in @msg.
 * Returns negative values on failure.
 *
 */
int nlbl_cipsov4_read(nlbl_socket sock, nlbl_data **msg, ssize_t *msg_len)
{
  int ret_val;
  nlbl_type nl_type;

  ret_val = nlbl_netlink_read(sock, &nl_type, msg, msg_len);
  if (ret_val >= 0 && nl_type != nlbl_cipsov4_fid)
      return -ENOMSG;

  return ret_val;
}

/*
 * NetLabel operations
 */


/**
 * nlbl_cipsov4_add_std - Add a standard CIPSOv4 label mapping
 * @sock: the NetLabel socket
 * @doi: the CIPSO DOI number
 * @tags: array of tag numbers
 * @lvls: array of level mappings
 * @cats: array of category mappings
 * 
 * Description:
 * Add the specified static CIPSO label mapping information to the NetLabel
 * system.  If @sock is zero then the function will handle opening and closing
 * it's own NETLINK socket.  Returns zero on success, negative values on
 * failure.
 *
 */
int nlbl_cipsov4_add_std(nlbl_socket sock,
                         cv4_doi doi,
                         cv4_tag_array *tags,
                         cv4_lvl_array *lvls,
                         cv4_cat_array *cats)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  unsigned int iter;
  unsigned int tmp_val;

  /* sanity checks */
  if (sock < 0 || tags == NULL || lvls == NULL || cats == NULL ||
      tags->size == 0 || lvls->size == 0 || cats->size == 0)
    return -EINVAL;

  /* open a socket if we need one */
  if (sock == 0) {
    ret_val = nlbl_netlink_open(&local_sock);
    if (ret_val < 0)
      return ret_val;
  }

  /* allocate a buffer for the message */
  msg_len = GENL_HDRLEN + 7 * NETLBL_LEN_U32 + NETLBL_LEN_U8 + NETLBL_LEN_U16 +
    tags->size * NETLBL_LEN_U8 +
    lvls->size * (NETLBL_LEN_U32 + NETLBL_LEN_U8) +
    cats->size * (NETLBL_LEN_U32 + NETLBL_LEN_U16);
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto add_std_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  nlbl_putinc_genlhdr(&msg_iter, NLBL_CIPSOV4_C_ADD);
  nlbl_putinc_u32(&msg_iter, doi, NULL);
  nlbl_putinc_u32(&msg_iter, CIPSO_V4_MAP_STD, NULL);
  nlbl_putinc_u32(&msg_iter, tags->size, NULL);
  for (iter = 0; iter < tags->size; iter++)
    nlbl_putinc_u8(&msg_iter, tags->array[iter], NULL);
  nlbl_putinc_u32(&msg_iter, lvls->size, NULL);
  for (iter = 0, tmp_val = 0; iter < lvls->size; iter++)
    if (lvls->array[iter * 2] > tmp_val)
      tmp_val = lvls->array[iter * 2];
  nlbl_putinc_u32(&msg_iter, tmp_val + 1, NULL);
  for (iter = 0, tmp_val = 0; iter < lvls->size; iter++)
    if (lvls->array[iter * 2 + 1] > tmp_val)
      tmp_val = lvls->array[iter * 2 + 1];
  nlbl_putinc_u8(&msg_iter, tmp_val + 1, NULL);
  nlbl_putinc_u32(&msg_iter, cats->size, NULL);
  for (iter = 0, tmp_val = 0; iter < cats->size; iter++)
    if (cats->array[iter * 2] > tmp_val)
      tmp_val = cats->array[iter * 2];
  nlbl_putinc_u32(&msg_iter, tmp_val + 1, NULL);
  for (iter = 0, tmp_val = 0; iter < cats->size; iter++)
    if (cats->array[iter * 2 + 1] > tmp_val)
      tmp_val = cats->array[iter * 2 + 1];
  nlbl_putinc_u16(&msg_iter, tmp_val + 1, NULL);
  for (iter = 0; iter < lvls->size; iter++) {
    nlbl_putinc_u32(&msg_iter, lvls->array[iter * 2], NULL);
    nlbl_putinc_u8(&msg_iter, lvls->array[iter * 2 + 1], NULL);
  }
  for (iter = 0; iter < cats->size; iter++) {
    nlbl_putinc_u32(&msg_iter, cats->array[iter * 2], NULL);
    nlbl_putinc_u16(&msg_iter, cats->array[iter * 2 + 1], NULL);
  }

  /* send the request */
  ret_val = nlbl_cipsov4_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto add_std_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_cipsov4_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto add_std_return;

  /* parse the response */
  ret_val = nlbl_common_ack_parse(msg + NLMSG_LENGTH(0),
				  msg_len - NLMSG_LENGTH(0),
				  NLBL_CIPSOV4_C_ACK);

 add_std_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_cipsov4_add_pass - Add a pass through CIPSOv4 label mapping
 * @sock: the NetLabel socket
 * @doi: the CIPSO DOI number
 * @tags: array of tag numbers
 * 
 * Description:
 * Add the specified pass through CIPSO label mapping information to the
 * NetLabel system.  If @sock is zero then the function will handle opening
 * and closing it's own NETLINK socket.  Returns zero on success, negative
 * values on failure.
 *
 */
int nlbl_cipsov4_add_pass(nlbl_socket sock,
			  cv4_doi doi,
			  cv4_tag_array *tags)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  unsigned int iter;

  /* sanity checks */
  if (sock < 0 || tags == NULL || tags->size == 0)
    return -EINVAL;

  /* open a socket if we need one */
  if (sock == 0) {
    ret_val = nlbl_netlink_open(&local_sock);
    if (ret_val < 0)
      return ret_val;
  }

  /* allocate a buffer for the message */
  msg_len = GENL_HDRLEN + 3 * NETLBL_LEN_U32 + tags->size * NETLBL_LEN_U8;
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto add_pass_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  nlbl_putinc_genlhdr(&msg_iter, NLBL_CIPSOV4_C_ADD);
  nlbl_putinc_u32(&msg_iter, doi, NULL);
  nlbl_putinc_u32(&msg_iter, CIPSO_V4_MAP_PASS, NULL);
  nlbl_putinc_u32(&msg_iter, tags->size, NULL);
  for (iter = 0; iter < tags->size; iter++)
    nlbl_putinc_u8(&msg_iter, tags->array[iter], NULL);

  /* send the request */
  ret_val = nlbl_cipsov4_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto add_pass_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_cipsov4_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto add_pass_return;

  /* parse the response */
  ret_val = nlbl_common_ack_parse(msg + NLMSG_LENGTH(0),
				  msg_len - NLMSG_LENGTH(0),
				  NLBL_CIPSOV4_C_ACK);

 add_pass_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_cipsov4_del - Delete a CIPSOv4 label mapping
 * @sock: the NetLabel socket
 * @doi: the CIPSO DOI number
 *
 * Description:
 * Remove the CIPSO label mapping with the DOI value matching @doi.  If @sock
 * is zero then the function will handle opening and closing it's own NETLINK
 * socket.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_cipsov4_del(nlbl_socket sock, cv4_doi doi)
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
    goto del_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  nlbl_putinc_genlhdr(&msg_iter, NLBL_CIPSOV4_C_REMOVE);
  nlbl_putinc_u32(&msg_iter, doi, NULL);

  /* send the request */
  ret_val = nlbl_cipsov4_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto del_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_cipsov4_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto del_return;

  /* parse the response */
  ret_val = nlbl_common_ack_parse(msg + NLMSG_LENGTH(0),
				  msg_len - NLMSG_LENGTH(0),
				  NLBL_CIPSOV4_C_ACK);

 del_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_cipsov4_list - List the details of a specific CIPSOv4 label mapping
 * @sock: the NetLabel socket
 * @doi: the CIPSO DOI number
 * @maptype: the DOI mapping type
 * @tags: array of tag numbers
 * @lvls: array of level mappings
 * @cats: array of category mappings
 *
 * Description:
 * Query the kernel for the specified CIPSOv4 mapping specified by @doi and
 * return the details of the mapping to the caller.  If @sock is zero then the
 * function will handle opening and closing it's own NETLINK socket.  Returns
 * zero on success, negative values on failure.
 *
 */
int nlbl_cipsov4_list(nlbl_socket sock,
                      cv4_doi doi,
		      cv4_maptype *maptype,
                      cv4_tag_array *tags,
                      cv4_lvl_array *lvls,
                      cv4_cat_array *cats)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;
  unsigned int iter;

  /* sanity checks */
  if (sock < 0 || tags == NULL || lvls == NULL || cats == NULL)
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
    goto list_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  nlbl_putinc_genlhdr(&msg_iter, NLBL_CIPSOV4_C_LIST);
  nlbl_putinc_u32(&msg_iter, doi, NULL);

  /* send the request */
  ret_val = nlbl_cipsov4_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto list_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_cipsov4_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto list_return;
  msg_iter = msg + NLMSG_LENGTH(0);
  msg_len -= NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  if (genl_hdr->cmd != NLBL_CIPSOV4_C_LIST) {
    ret_val = -ENOMSG;
    goto list_return;
  }
  msg_iter += GENL_HDRLEN;
  msg_len -= GENL_HDRLEN;
  *maptype = nlbl_getinc_u32(&msg_iter, &msg_len);
  switch (*maptype) {
  case CIPSO_V4_MAP_STD:
    tags->size = nlbl_getinc_u32(&msg_iter, &msg_len);
    lvls->size = nlbl_getinc_u32(&msg_iter, &msg_len);
    cats->size = nlbl_getinc_u32(&msg_iter, &msg_len);
    tags->array = malloc(sizeof(cv4_tag) * tags->size);
    lvls->array = malloc(sizeof(cv4_lvl) * lvls->size * 2);
    cats->array = malloc(sizeof(cv4_cat) * cats->size * 2);
    if (tags->array == NULL || lvls->array == NULL || cats->array == NULL) {
      ret_val = -ENOMEM;
      goto list_return;
    }
    for (iter = 0; iter < tags->size; iter++)
      tags->array[iter] = nlbl_getinc_u8(&msg_iter, &msg_len);
    for (iter = 0; iter < lvls->size; iter++) {
      lvls->array[2 * iter] = nlbl_getinc_u32(&msg_iter, &msg_len);
      lvls->array[2 * iter + 1] = nlbl_getinc_u8(&msg_iter, &msg_len);
    }
    for (iter = 0; iter < cats->size; iter++) {
      cats->array[2 * iter] = nlbl_getinc_u32(&msg_iter, &msg_len);
      cats->array[2 * iter + 1] = nlbl_getinc_u16(&msg_iter, &msg_len);
    }
    break;
  case CIPSO_V4_MAP_PASS:
    tags->size = nlbl_getinc_u32(&msg_iter, &msg_len);
    tags->array = malloc(sizeof(cv4_tag) * tags->size);
    if (tags->array == NULL) {
      ret_val = -ENOMEM;
      goto list_return;
    }
    for (iter = 0; iter < tags->size; iter++)
      tags->array[iter] = nlbl_getinc_u8(&msg_iter, &msg_len);
    break;
  default:
    ret_val = -ENOMSG;
    goto list_return;
  }

  ret_val = 0;

 list_return:
  if (ret_val < 0) {
    tags->size = 0;
    if (tags->array)
      free(tags->array);
    lvls->size = 0;
    if (lvls->array)
      free(lvls->array);
    cats->size = 0;
    if (cats->array)
      free(cats->array);
  }
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_cipsov4_list_all - List the CIPSOv4 label mappings
 * @sock: the NetLabel socket
 * @doi_list: a list of DOI values
 * @mtype_list: a list of the mapping types
 *
 * Description:
 * Query the kernel for the configured CIPSOv4 mappings and return two lists;
 * @doi_list which contains the DOI values and @mtype_list which contains the
 * type of mapping.  If @sock is zero then the function will handle opening and
 * closing it's own NETLINK socket.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_cipsov4_list_all(nlbl_socket sock, 
                          cv4_doi **doi_list,
                          cv4_maptype **mtype_list,
                          size_t *count)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;
  size_t doi_count;
  cv4_doi *list_doi = NULL;
  cv4_maptype *list_mtype = NULL;
  unsigned int iter;

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
  msg_len = GENL_HDRLEN;
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto list_all_return;
  }
  memset(msg, 0, msg_len);

  /* write the message into the buffer */
  nlbl_put_genlhdr(msg, NLBL_CIPSOV4_C_LISTALL);

  /* send the request */
  ret_val = nlbl_cipsov4_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto list_all_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_cipsov4_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto list_all_return;
  msg_iter = msg + NLMSG_LENGTH(0);
  msg_len -= NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  if (genl_hdr->cmd != NLBL_CIPSOV4_C_LISTALL) {
    ret_val = -ENOMSG;
    goto list_all_return;
  }
  msg_iter += GENL_HDRLEN;
  msg_len -= GENL_HDRLEN;
  doi_count = nlbl_getinc_u32(&msg_iter, &msg_len);
  list_doi = malloc(sizeof(cv4_doi) * doi_count);
  list_mtype = malloc(sizeof(cv4_maptype) * doi_count);
  if (list_doi == NULL || list_mtype == NULL) {
    ret_val = -ENOMEM;
    goto list_all_return;
  }
  for (iter = 0; iter < doi_count; iter++) {
    list_doi[iter] = nlbl_getinc_u32(&msg_iter, &msg_len);
    list_mtype[iter] = nlbl_getinc_u32(&msg_iter, &msg_len);
  }

  *count = doi_count;
  *doi_list = list_doi;
  *mtype_list = list_mtype;

  ret_val = 0;

 list_all_return:
  if (ret_val < 0) {
    if (list_doi)
      free(list_doi);
    if (list_mtype)
      free(list_mtype);
  }
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

