/*
 * Management Module Functions
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
static nlbl_type nlbl_mgmt_fid = -1;

/*
 * Init functions
 */


/**
 * nlbl_mgmt_init - Perform any setup needed
 *
 * Description:
 * Do any setup needed for the management component, including determining the
 * NetLabel Mangement Generic NETLINK family ID.  Returns zero on success,
 * negative values on error.
 *
 */
int nlbl_mgmt_init(void)
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
  msg_len = GENL_HDRLEN + NLA_HDRLEN + strlen(NETLBL_NLTYPE_MGMT_NAME) + 1;
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
  nla_hdr->nla_len = NLA_HDRLEN + strlen(NETLBL_NLTYPE_MGMT_NAME) + 1;
  nla_hdr->nla_type = CTRL_ATTR_FAMILY_NAME;
  msg_iter += NLA_HDRLEN;
  strcpy((char *)msg_iter, NETLBL_NLTYPE_MGMT_NAME);

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
  nlbl_mgmt_fid = nlbl_get_u16((unsigned char *)nla_hdr);

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
 * nlbl_mgmt_write - Send a NetLabel management message
 * @sock: the socket
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Write the message in @msg to the NetLabel socket @sock.  Returns zero on
 * success, negative values on failure.
 *
 */
int nlbl_mgmt_write(nlbl_socket sock, nlbl_data *msg, size_t msg_len)
{
  return nlbl_netlink_write(sock, nlbl_mgmt_fid, msg, msg_len);
}

/**
 * nlbl_mgmt_read - Read a NetLbel management message
 * @sock: the socket
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Try to read a NetLabel management message and return the message in @msg.
 * Returns negative values on failure.
 *
 */
int nlbl_mgmt_read(nlbl_socket sock, nlbl_data **msg, ssize_t *msg_len)
{
  int ret_val;
  nlbl_type nl_type;

  ret_val = nlbl_netlink_read(sock, &nl_type, msg, msg_len);
  if (ret_val >= 0 && nl_type != nlbl_mgmt_fid)
      return -ENOMSG;

  return ret_val;
}

/*
 * NetLabel operations
 */


/**
 * nlbl_mgmt_modules - Determine the supported list of NetLabel modules
 * @sock: the NetLabel socket
 * @module_list: the module list
 * @module_count: the number of modules in the list
 *
 * Description:
 * Request a list of supported NetLabel modules from the kernel and return the
 * result to the caller.  If @sock is zero then the function will handle
 * opening and closing it's own NETLINK socket.  Returns zero on success,
 * negative values on failure.
 *
 */
int nlbl_mgmt_modules(nlbl_socket sock,
                      unsigned int **module_list,
                      size_t *module_count)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;
  unsigned int *list;
  unsigned int mod_count;
  int iter;

  /* sanity checks */
  if (sock < 0 || module_list == NULL || module_count == NULL)
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
    goto modules_return;
  }
  memset(msg, 0, msg_len);

  /* write the message into the buffer */
  nlbl_put_genlhdr(msg, NLBL_MGMT_C_MODULES);
  
  /* send the request */
  ret_val = nlbl_mgmt_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto modules_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_mgmt_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto modules_return;
  msg_iter = msg + NLMSG_LENGTH(0);
  msg_len -= NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  if (genl_hdr->cmd != NLBL_MGMT_C_MODULES) {
    ret_val = -ENOMSG;
    goto modules_return;
  }
  msg_iter += GENL_HDRLEN;
  msg_len -= GENL_HDRLEN;
  if (msg_len < NETLBL_LEN_U32) {
    ret_val = -ENOMSG;
    goto modules_return;
  }
  mod_count = nlbl_getinc_u32(&msg_iter, &msg_len);
  if (mod_count == 0 || msg_len < mod_count * NETLBL_LEN_U32) {
    ret_val = -ENOMSG;
    goto modules_return;
  }
  
  /* return the list of modules */
  list = malloc(mod_count * NETLBL_LEN_U32);
  if (list == NULL) {
    ret_val = -ENOMEM;
    goto modules_return;
  }
  for (iter = 0; iter < mod_count; iter++)
    list[iter] = nlbl_getinc_u32(&msg_iter, &msg_len);
  *module_count = mod_count;
  *module_list = list;

  ret_val = 0;

 modules_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_mgmt_version - Determine the kernel's NetLabel version
 * @sock: the NetLabel socket
 * @ver: the version string buffer
 * @ver_len: the length of the version string in bytes
 *
 * Description:
 * Request the NetLabel version string from the kernel and return the result
 * to the caller.  If @sock is zero then the function will handle opening and
 * closing it's own NETLINK socket.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_mgmt_version(nlbl_socket sock, unsigned int *version)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;

  /* sanity checks */
  if (sock < 0 || version == NULL)
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
    goto version_return;
  }
  memset(msg, 0, msg_len);

  /* write the message into the buffer */
  nlbl_put_genlhdr(msg, NLBL_MGMT_C_VERSION);
  
  /* send the request */
  ret_val = nlbl_mgmt_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto version_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_mgmt_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto version_return;
  msg_iter = msg + NLMSG_LENGTH(0);
  msg_len -= NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  if (genl_hdr->cmd != NLBL_MGMT_C_VERSION) {
    ret_val = -ENOMSG;
    goto version_return;
  }
  msg_iter += GENL_HDRLEN;
  msg_len -= GENL_HDRLEN;
  if (msg_len != NETLBL_LEN_U32) {
    ret_val = -ENOMSG;
    goto version_return;
  }
  *version = nlbl_get_u32(msg_iter);

  ret_val = 0;

 version_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_mgmt_add - Add a domain mapping to the NetLabel system
 * @sock: the NetLabel socket
 * @domain_list: list of domain mappings
 * @domain_count: number of entries in the domain mapping list
 * @def_flag: default mapping flag 
 *
 * Description:
 * Add the domain mappings in @domain_list to the NetLabel system.  If @sock is
 * zero then the function will handle opening and closing it's own NETLINK
 * socket.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_mgmt_add(nlbl_socket sock,
                  mgmt_domain *domain_list,
                  size_t domain_count,
                  unsigned int def_flag)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  unsigned int iter;

  /* sanity checks */
  if (sock < 0 || domain_list == NULL || domain_count < 1 || 
      (def_flag && domain_count != 1))
    return -EINVAL;

  /* open a socket if we need one */
  if (sock == 0) {
    ret_val = nlbl_netlink_open(&local_sock);
    if (ret_val < 0)
      return ret_val;
  }

  /* allocate a buffer for the message */
  msg_len = GENL_HDRLEN + (def_flag ? 0 : NETLBL_LEN_U32);
  for (iter = 0; iter < domain_count; iter++) {
    msg_len += NETLBL_LEN_U32;
    if (!def_flag)
      msg_len += nlbl_len_payload(domain_list[iter].domain_len);
    switch (domain_list[iter].proto_type)
      {
      case NETLBL_NLTYPE_UNLABELED:
        break;
      case NETLBL_NLTYPE_CIPSOV4:
        msg_len += NETLBL_LEN_U32;
        break;
      default:
        ret_val = -EINVAL;
        goto add_return;
      }
  }
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto add_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  if (!def_flag) {
    nlbl_putinc_genlhdr(&msg_iter, NLBL_MGMT_C_ADD);
    nlbl_putinc_u32(&msg_iter, domain_count, NULL);
  } else
    nlbl_putinc_genlhdr(&msg_iter, NLBL_MGMT_C_ADDDEF);
  for (iter = 0; iter < domain_count; iter++) {
    if (!def_flag)
      nlbl_putinc_str(&msg_iter, domain_list[iter].domain, NULL);
    nlbl_putinc_u32(&msg_iter, domain_list[iter].proto_type, NULL);
    switch (domain_list[iter].proto_type) {
    case NETLBL_NLTYPE_UNLABELED:
      break;
    case NETLBL_NLTYPE_CIPSOV4:
      nlbl_putinc_u32(&msg_iter, domain_list[iter].proto.cipsov4.doi, NULL);
      break;
    default:
      ret_val = -EINVAL;
      goto add_return;
    }
  }

  /* send the request */
  ret_val = nlbl_mgmt_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto add_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_mgmt_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto add_return;

  /* parse the response */
  ret_val = nlbl_common_ack_parse(msg + NLMSG_LENGTH(0),
				  msg_len - NLMSG_LENGTH(0),
				  NLBL_MGMT_C_ACK);

 add_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);

  return ret_val;
}

/**
 * nlbl_mgmt_del - Remove a domain mapping from the NetLabel system
 * @sock: the NetLabel socket
 * @domains: list of domain strings
 * @domain_len: list of domain string lengths
 * @domain_count: the number of domain strings
 * @def_flag: default mapping flag 
 *
 * Description:
 * Remove the domain mappings in @domains from the NetLabel system.  If @sock
 * is zero then the function will handle opening and closing it's own NETLINK
 * socket.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_mgmt_del(nlbl_socket sock,
		  char **domains,
		  size_t *domain_len,
		  size_t domain_count,
		  unsigned int def_flag)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  unsigned int iter;

  /* sanity checks */
  if (sock < 0 || (!def_flag && (domains == NULL || domain_len == NULL || 
				 domain_count < 1)))
    return -EINVAL;

  /* open a socket if we need one */
  if (sock == 0) {
    ret_val = nlbl_netlink_open(&local_sock);
    if (ret_val < 0)
      return ret_val;
  }

  /* allocate a buffer for the message */
  if (!def_flag) {
    msg_len = GENL_HDRLEN + NETLBL_LEN_U32;
    for (iter = 0; iter < domain_count; iter++)
      msg_len += nlbl_len_payload(domain_len[iter]);
  } else
    msg_len = GENL_HDRLEN;
  msg = malloc(msg_len);
  if (msg == NULL) {
    ret_val = -ENOMEM;
    goto del_return;
  }
  memset(msg, 0, msg_len);
  msg_iter = msg;

  /* write the message into the buffer */
  if (!def_flag) {
    nlbl_putinc_genlhdr(&msg_iter, NLBL_MGMT_C_REMOVE);
    nlbl_putinc_u32(&msg_iter, domain_count, NULL);
    for (iter = 0; iter < domain_count; iter++)
      nlbl_putinc_str(&msg_iter, domains[iter], NULL);
  } else
    nlbl_putinc_genlhdr(&msg_iter, NLBL_MGMT_C_REMOVEDEF);

  /* send the request */
  ret_val = nlbl_mgmt_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto del_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_mgmt_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto del_return;

  /* parse the response */
  ret_val = nlbl_common_ack_parse(msg + NLMSG_LENGTH(0),
				  msg_len - NLMSG_LENGTH(0),
				  NLBL_MGMT_C_ACK);

 del_return:
  if (msg)
    free(msg);
  if (sock == 0)
    nlbl_netlink_close(local_sock);
    
  return ret_val;
}

/**
 * nlbl_mgmt_list - List the NetLabel domain mappings
 * @sock: the NetLabel socket
 * @domain_list: list of domain mappings
 * @domain_count: number of domains in the list
 * @def_flag: default mapping flag
 *
 * Description:
 * List the configured domain mappings in the NetLabel system.  If @sock is
 * zero then the function will handle opening and closing it's own NETLINK
 * socket.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_mgmt_list(nlbl_socket sock, 
		   mgmt_domain **domain_list,
		   size_t *domain_count,
		   unsigned int def_flag)
{
  int ret_val = -EPERM;
  nlbl_socket local_sock = sock;
  nlbl_data *msg = NULL;
  nlbl_data *msg_iter;
  ssize_t msg_len;
  struct genlmsghdr *genl_hdr;
  unsigned int iter;
  mgmt_domain *domains = NULL;

  /* sanity checks */
  if (sock < 0 || domain_list == NULL || domain_count == NULL)
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
  if (!def_flag)
    nlbl_put_genlhdr(msg, NLBL_MGMT_C_LIST);
  else
    nlbl_put_genlhdr(msg, NLBL_MGMT_C_LISTDEF);

  /* send the request */
  ret_val = nlbl_mgmt_write(local_sock, msg, msg_len);
  if (ret_val < 0)
    goto list_return;
  free(msg);
  msg = NULL;
  msg_len = 0;

  /* read the results */
  ret_val = nlbl_mgmt_read(local_sock, &msg, &msg_len);
  if (ret_val < 0)
    goto list_return;
  msg_iter = msg + NLMSG_LENGTH(0);
  msg_len -= NLMSG_LENGTH(0);

  /* parse the response */
  genl_hdr = (struct genlmsghdr *)msg_iter;
  if (!def_flag) {
    if (genl_hdr->cmd != NLBL_MGMT_C_LIST) {
      ret_val = -ENOMSG;
      goto list_return;
    }
  } else {
    if (genl_hdr->cmd != NLBL_MGMT_C_LISTDEF) {
      ret_val = -ENOMSG;
      goto list_return;
    }
  }
  msg_iter += GENL_HDRLEN;
  msg_len -= GENL_HDRLEN;
  if (!def_flag)
    *domain_count = nlbl_getinc_u32(&msg_iter, &msg_len);
  else
    *domain_count = 1;
  for (iter = 0; iter < *domain_count; iter++) {
    domains = realloc(domains, sizeof(mgmt_domain) * (iter + 1));
    if (domains == NULL) {
      ret_val = -ENOMEM;
      goto list_return;
    }
    if (!def_flag) {
      domains[iter].domain_len = nlbl_get_len(msg_iter);
      domains[iter].domain = nlbl_getinc_str(&msg_iter, &msg_len);
      if (domains[iter].domain == NULL) {
	ret_val = -ENOMEM;
	goto list_return;
      }
    } else {
      domains[iter].domain = NULL;
      domains[iter].domain_len = 0;
    }
    domains[iter].proto_type = nlbl_getinc_u32(&msg_iter, &msg_len);
    switch (domains[iter].proto_type) {
    case NETLBL_NLTYPE_UNLABELED:
      break;
    case NETLBL_NLTYPE_CIPSOV4:
      domains[iter].proto.cipsov4.maptype = nlbl_getinc_u32(&msg_iter,
							      &msg_len);
      domains[iter].proto.cipsov4.doi = nlbl_getinc_u32(&msg_iter,
							  &msg_len);
      break;
    default:
      if (def_flag) {
	free(domains);
	*domain_count = 0;
	domains = NULL;
	ret_val = 0;
      } else
	ret_val = -EBADMSG;
      goto list_return;
    }
  }
  *domain_list = domains;
 
  ret_val = 0;
 
 list_return:
  if (msg)
    free(msg);
  if (ret_val < 0) {
    if (domains) {
      for (iter = 0; iter < *domain_count; iter++) {
	if (domains[iter].domain)
	  free(domains[iter].domain);
      }
      free(domains);
    }
  }
  if (sock == 0)
    nlbl_netlink_close(local_sock);
 
  return ret_val;
}
