/*
 * NetLabel Low-Level Communication Functions
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>

#include <libnetlabel.h>

#include "netlabel_internal.h"

/* Netlink read timeout (in seconds) */
static uint32_t nlcomm_read_timeout = 10;

/*
 * Helper Functions
 */

/**
 * nlbl_comm_hndl_valid - Validate a NetLabel handle
 * @hndl: the NetLabel handle
 *
 * Description:
 * Return true if @hndl is valid, false otherwise.
 *
 */
static int nlbl_comm_hndl_valid(nlbl_handle *hndl)
{
  return (hndl != NULL && hndl->nl_hndl != NULL);
}

/*
 * Control Functions
 */

/**
 * nlbl_comm_timeout - Set the NetLabel timeout
 * @seconds: the timeout in seconds
 *
 * Description:
 * Set the timeout value used by the NetLabel communications layer.
 *
 */
void nlbl_comm_timeout(uint32_t seconds)
{
  nlcomm_read_timeout = seconds;
}

/*
 * Communication Functions
 */

/**
 * nlbl_comm_open - Create and bind a NetLabel handle
 *
 * Description:
 * Create a new NetLabel handle, bind it to the running process, and connect to
 * the Generic Netlink subsystem.  Returns a pointer to the NetLabel handle
 * structure.
 *
 */
nlbl_handle *nlbl_comm_open(void)
{
  nlbl_handle *hndl;

  /* allocate the handle memory */
  hndl = calloc(1, sizeof(*hndl));
  if (hndl == NULL)
    goto open_failure;

  /* create a new netlink handle */
  hndl->nl_hndl = nl_handle_alloc();
  if (hndl->nl_hndl == NULL)
    goto open_failure;

  /* set the netlink handle properties */
#if LIBNL_VERSION >= 1006
  nl_handle_set_peer_pid(hndl->nl_hndl, 0);
  nl_set_passcred(hndl->nl_hndl, 1);
#endif
  nl_disable_sequence_check(hndl->nl_hndl);

  /* connect to the generic netlink subsystem in the kernel */
  if (nl_connect(hndl->nl_hndl, NETLINK_GENERIC) != 0)
    goto open_failure;

  return hndl;

 open_failure:
  if (hndl) {
    if (hndl->nl_hndl) {
      nl_close(hndl->nl_hndl);
      nl_handle_destroy(hndl->nl_hndl);
    }
    free(hndl);
  }
  return NULL;
}

/**
 * nlbl_comm_close - Close and destroy a NetLabel handle
 * @hndl: the NetLabel handle
 *
 * Description:
 * Closes the given NetLabel socket.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_comm_close(nlbl_handle *hndl)
{
  /* sanity checks */
  if (!nlbl_comm_hndl_valid(hndl))
    return -EINVAL;

  /* close and destroy the socket */
  nl_close(hndl->nl_hndl);
  nl_handle_destroy(hndl->nl_hndl);

  /* free the memory */
  free(hndl);

  return 0;
}

/**
 * nlbl_comm_recv_raw - Read a message from a NetLabel handle
 * @hndl: the NetLabel handle
 * @data: the message buffer
 *
 * Description:
 * Reads a message from the NetLabel handle and stores it the pointer returned
 * in @msg.  This function allocates space for @msg, making the caller
 * responsibile for freeing @msg later.  Returns the number of bytes read on
 * success, zero on EOF, and negative values on failure.
 *
 */
int nlbl_comm_recv_raw(nlbl_handle *hndl, unsigned char **data)
{
  int ret_val;
  struct sockaddr_nl peer_nladdr;
  struct ucred *creds = NULL;
  int nl_fd;
  fd_set read_fds;
  struct timeval timeout;

  /* sanity checks */
  if (!nlbl_comm_hndl_valid(hndl) || data == NULL)
    return -EINVAL;

  /* we use blocking sockets so do enforce a timeout using select() if no data
   * is waiting to be read from the handle */
  timeout.tv_sec = nlcomm_read_timeout;
  timeout.tv_usec = 0;
  nl_fd = nl_handle_get_fd(hndl->nl_hndl);
  FD_ZERO(&read_fds);
  FD_SET(nl_fd, &read_fds);
  ret_val = select(nl_fd + 1, &read_fds, NULL, NULL, &timeout);
  if (ret_val < 0)
    return -errno;
  else if (ret_val == 0)
    return -EAGAIN;

  /* perform the read operation */
#if LIBNL_VERSION == 1005
  ret_val = nl_recv(hndl->nl_hndl, &peer_nladdr, data);
  if (ret_val < 0)
    return ret_val;
  /* XXX - avoid a compiler warning about unused variables */
  creds = NULL;
#elif LIBNL_VERSION >= 1006
  ret_val = nl_recv(hndl->nl_hndl, &peer_nladdr, data, &creds);
  if (ret_val < 0)
    return ret_val;
#endif

  /* if we are setup to receive credentials, only accept messages from the
   * kernel (ignore all others and send an -EAGAIN) */
  if (creds != NULL && creds->pid != 0) {
    ret_val = -EAGAIN;
    goto recv_raw_failure;
  }

  return ret_val;

 recv_raw_failure:
  free(data);
  return ret_val;
}

/**
 * nlbl_comm_recv - Read a message from a NetLabel handle
 * @hndl: the NetLabel handle
 * @msg: the message buffer
 *
 * Description:
 * Reads a message from the NetLabel handle and stores it the pointer returned
 * in @msg.  This function allocates space for @msg, making the caller
 * responsibile for freeing @msg later.  Returns the number of bytes read on
 * success, zero on EOF, and negative values on failure.
 *
 */
int nlbl_comm_recv(nlbl_handle *hndl, nlbl_msg **msg)
{
  int ret_val;
  struct sockaddr_nl peer_nladdr;
  struct ucred *creds = NULL;
  int nl_fd;
  fd_set read_fds;
  struct timeval timeout;
  unsigned char *data;

  /* sanity checks */
  if (!nlbl_comm_hndl_valid(hndl) || msg == NULL)
    return -EINVAL;

  /* we use blocking sockets so do enforce a timeout using select() if no data
   * is waiting to be read from the handle */
  timeout.tv_sec = nlcomm_read_timeout;
  timeout.tv_usec = 0;
  nl_fd = nl_handle_get_fd(hndl->nl_hndl);
  FD_ZERO(&read_fds);
  FD_SET(nl_fd, &read_fds);
  ret_val = select(nl_fd + 1, &read_fds, NULL, NULL, &timeout);
  if (ret_val < 0)
    return -errno;
  else if (ret_val == 0)
    return -EAGAIN;

  /* perform the read operation */
#if LIBNL_VERSION == 1005
  ret_val = nl_recv(hndl->nl_hndl, &peer_nladdr, &data);
  if (ret_val < 0)
    return ret_val;
  /* XXX - avoid a compiler warning about unused variables */
  creds = NULL;
#elif LIBNL_VERSION >= 1006
  ret_val = nl_recv(hndl->nl_hndl, &peer_nladdr, &data, &creds);
  if (ret_val < 0)
    return ret_val;
#endif

  /* if we are setup to receive credentials, only accept messages from the
   * kernel (ignore all others and send an -EAGAIN) */
  if (creds != NULL && creds->pid != 0) {
    ret_val = -EAGAIN;
    goto recv_failure;
  }

  /* make sure the received buffer is the correct length */
  if (!nlmsg_ok((struct nlmsghdr *)data, ret_val)) {
    ret_val = -EBADMSG;
    goto recv_failure;
  }

  /* convert the received buffer into a nlbl_msg */
  *msg = nlmsg_convert((struct nlmsghdr *)data);
  if (*msg == NULL) {
    ret_val = -EBADMSG;
    goto recv_failure;
  }

  return ret_val;

 recv_failure:
  free(data);
  return ret_val;
}

/**
 * nlbl_comm_send - Write a message to a NetLabel handle
 * @hndl: the NetLabel handle
 * @msg: the message
 *
 * Description:
 * Write the message in @msg to the NetLabel handle @hndl.  Returns the number
 * of bytes written on success, or negative values on failure.
 *
 */
int nlbl_comm_send(nlbl_handle *hndl, nlbl_msg *msg)
{
  struct nlmsghdr *nl_hdr;
  struct ucred creds;

  /* sanity checks */
  if (!nlbl_comm_hndl_valid(hndl) || msg == NULL)
    return -EINVAL;

  /* setup our credentials using the _effective_ values */
  /* XXX - should we use the _real_ values instead? */
  creds.pid = getpid();
  creds.uid = geteuid();
  creds.gid = getegid();

  /* set the message properties */
#if LIBNL_VERSION >= 1006
  nlmsg_set_creds(msg, &creds);
#endif

  /* request a netlink ack message */
  nl_hdr = nlbl_msg_nlhdr(msg);
  if (nl_hdr == NULL)
    return -EBADMSG;
  nl_hdr->nlmsg_flags |= NLM_F_ACK;

  /* send the message */
#if LIBNL_VERSION == 1005
  return nl_send_auto_complete(hndl->nl_hndl, nlbl_msg_nlhdr(msg));
#elif LIBNL_VERSION >= 1006
  return = nl_send_auto_complete(hndl->nl_hndl, msg);
#endif
}
