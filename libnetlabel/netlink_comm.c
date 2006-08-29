/*
 * NETLINK Communication Functions
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/netlabel.h>

#include <libnetlabel.h>

/* default buffer size */
#define NLCOMM_BUF_SIZE 8192

/* NETLINK sequence number */
static unsigned int nlcomm_seqnum = 0;

/* NETLINK read timeout (in seconds) */
static unsigned int nlcomm_read_timeout = 10;

/**
 * nlbl_netlink_timeout - Set the NETLINK timeout
 * @seconds: the timeout in seconds
 *
 * Description:
 * Set the timeout value used by the NETLINK communications layer.
 *
 */
void nlbl_netlink_timeout(unsigned int seconds)
{
  nlcomm_read_timeout = seconds;
}

/**
 * nlbl_netlink_msgspace - Determine the correct size for a NETLINK payload
 * @len: the payload size
 *
 * Description:
 * Calculate the correct size for a NETLINK payload buffer.
 *
 */
size_t nlbl_netlink_msgspace(size_t len)
{
  return NLMSG_ALIGN(len);
}

/**
 * nlbl_netlink_open - Create and bind a NETLINK socket
 * @sock: the socket
 *
 * Description:
 * Create a new NETLINK socket and bind it to the running process.  Returns
 * zero on success, negative values on failure.
 *
 */
int nlbl_netlink_open(nlbl_socket *sock)
{
  nlbl_socket new_sock;
  struct sockaddr_nl nlbl_addr;

  /* sanity checks */
  if (sock == NULL)
    return -EINVAL;

  /* open a NetLabel socket */
  new_sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
  if (new_sock == -1)
    return -errno;

  /* bind the NetLabel socket */
  memset(&nlbl_addr, 0, sizeof(struct sockaddr_nl));
  nlbl_addr.nl_family = AF_NETLINK;
  nlbl_addr.nl_pid = getpid();
  if (bind(new_sock, (struct sockaddr *)&nlbl_addr, sizeof(nlbl_addr)) != 0) {
    close(new_sock);
    return -errno;
  }

  *sock = new_sock;
  return 0;
}

/**
 * nlbl_netlink_close - Closes and destroys a NETLINK socket
 * @sock: the socket
 *
 * Description:
 * Closes the given NETLINK socket.  Returns zero on success, negative values
 * on failure.
 *
 */
int nlbl_netlink_close(nlbl_socket sock)
{
  /* sanity checks */
  if (sock < 0)
    return -EINVAL;

  /* close the socket */
  if (close(sock) != 0)
    return -errno;

  return 0;
}

/**
 * nlbl_netlink_read - Read a message from a NETLINK socket
 * @sock: the socket
 * @msg: the message buffer
 * @msg_len: length of the buffer or message
 *
 * Description:
 * Reads a message from the NETLINK socket.  If the caller allocates it's own
 * buffer it should set @msg to point to that buffer and @msg_len should be set
 * to the size of the buffer.  If the caller wants the function to allocate the
 * buffer (using default values) it should set @msg_len equal to zero.  The
 * received message size is returned in @msg_len regardless of who allocates
 * the buffer.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_netlink_read(nlbl_socket sock, 
                      nlbl_type *type, 
                      nlbl_data **msg, 
                      ssize_t *msg_len)
{
  int ret_val;
  struct sockaddr_nl nlbl_addr;
  struct msghdr msg_hdr;
  struct iovec msg_iovec;
  nlbl_data *buf = NULL;
  size_t buf_len;
  ssize_t rcv_len;
  fd_set read_fds;
  struct timeval timeout;

  /* sanity checks */
  if (sock < 0 || msg == NULL || msg_len == NULL)
    return -EINVAL;

  /* get a buffer */
  if (*msg_len == 0) {
    /* we need to allocate a buffer */
    buf = malloc(NLCOMM_BUF_SIZE);
    if (buf == NULL)
      return -ENOMEM;
    buf_len = NLCOMM_BUF_SIZE;
  } else {
    /* caller already allocated a buffer for us */
    buf = *msg;
    buf_len = *msg_len;
  }
   memset(buf, 0, buf_len);

  /* setup the message buffer */
  msg_iovec.iov_base = (void *)buf;
  msg_iovec.iov_len = buf_len;

  /* setup the message header */
  msg_hdr.msg_name = (void *)&nlbl_addr;
  msg_hdr.msg_namelen = sizeof(struct sockaddr_nl);
  msg_hdr.msg_iov = &msg_iovec;
  msg_hdr.msg_iovlen = 1;

  /* do a select() until the socket has data or we hit our timeout */
  timeout.tv_sec = nlcomm_read_timeout;
  timeout.tv_usec = 0;
  FD_ZERO(&read_fds);
  FD_SET(sock, &read_fds);
  ret_val = select(sock + 1, &read_fds, NULL, NULL, &timeout);
  if (ret_val <= 0) {
    if (buf != *msg)
      free(buf);
    if (ret_val == 0)
      return -EAGAIN;
    return -errno;
  }

  /* do the read */
  rcv_len = recvmsg(sock, &msg_hdr, 0);
  if (rcv_len < 0 || (msg_hdr.msg_flags & MSG_TRUNC)) {
    if (buf != *msg)
      free(buf);
    return -errno;
  }

#if 0
  /* dump the raw message */
  {
    unsigned int iter;

    printf("DEBUG: dumping raw incoming NetLabel message, %u bytes\n",
           rcv_len);
    for (iter = 0; iter < rcv_len; iter++) {
      if (iter % 16 == 0)
        printf(" %.8u:", iter);
      if (iter % 4 == 0)
        printf(" ");
      printf("%.2x", buf[iter]);
      if ((iter + 1) % 16 == 0)
        printf("\n");
    }
    printf("\n");
  }
#endif

  /* set the return values */
  *type = ((struct nlmsghdr *)buf)->nlmsg_type;
  if (buf != *msg)
    *msg = buf;
  *msg_len = rcv_len;

  return 0;
}

/**
 * nlbl_netlink_write - Write a message to a NETLINK socket
 * @sock: the socket
 * @type: the message type
 * @msg: the message
 * @msg_len: the message length
 *
 * Description:
 * Write the message in @msg to the NETLINK socket @sock.  Returns zero on
 * success, negative values on failure.
 *
 */
int nlbl_netlink_write(nlbl_socket sock, 
                       nlbl_type type, 
                       nlbl_data *msg, 
                       size_t msg_len)
{
  struct sockaddr_nl nlbl_addr;
  struct msghdr msg_hdr;
  struct nlmsghdr *msg_nlhdr;
  struct iovec msg_iovec[2];
  ssize_t snd_len;

  /* sanity checks */
  if (sock < 0 || msg == NULL || msg_len <= 0)
    return -EINVAL;

#if 0
  /* dump the raw message */
  {
    unsigned int iter;

    printf("DEBUG: dumping raw outgoing NetLabel message, %u bytes\n",
           msg_len);
    for (iter = 0; iter < msg_len; iter++) {
      if (iter % 16 == 0)
        printf(" %.8u:", iter);
      if (iter % 4 == 0)
        printf(" ");
      printf("%.2x", msg[iter]);
      if ((iter + 1) % 16 == 0)
        printf("\n");
    }
    printf("\n");
  }
#endif

  /* allocate the NETLINK message header + padding */
  msg_nlhdr = malloc(NLMSG_LENGTH(0));
  if (msg_nlhdr == NULL)
    return -ENOMEM;

  /* setup the NETLINK message header */
  msg_nlhdr->nlmsg_len = NLMSG_LENGTH(msg_len);
  msg_nlhdr->nlmsg_pid = getpid();
  msg_nlhdr->nlmsg_flags = NLM_F_REQUEST;
  msg_nlhdr->nlmsg_seq = nlcomm_seqnum++;
  msg_nlhdr->nlmsg_type = type;

  /* setup the message buffer */
  msg_iovec[0].iov_base = (void *)msg_nlhdr;
  msg_iovec[0].iov_len = NLMSG_LENGTH(0);
  msg_iovec[1].iov_base = (void *)msg;
  msg_iovec[1].iov_len = msg_len;

  /* setup the message header */
  memset(&nlbl_addr, 0, sizeof(struct sockaddr_nl));
  nlbl_addr.nl_family = AF_NETLINK;
  memset(&msg_hdr, 0, sizeof(struct msghdr));
  msg_hdr.msg_name = (void *)&nlbl_addr;
  msg_hdr.msg_namelen = sizeof(struct sockaddr_nl);
  msg_hdr.msg_iov = msg_iovec;
  msg_hdr.msg_iovlen = 2;

  /* do the write */
  snd_len = sendmsg(sock, &msg_hdr, 0);

  free(msg_nlhdr);
  if (snd_len < NLMSG_LENGTH(msg_len))
    return -errno;
  return 0;
}

