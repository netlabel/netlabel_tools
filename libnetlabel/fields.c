/*
 * NetLabel Library Field Functions
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
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/netlabel.h>

#include <libnetlabel.h>

/**
 * nlbl_put_hdr - Write a NETLINK header into a buffer
 * @buffer: the buffer
 * @msg_type: the NETLINK message type
 * @msg_len: the NETLINK message length
 * @msg_flags: the NETLINK message flags
 * @msg_pid: the NETLINK message PID
 * @msg_seq: the NETLINK message sequence number
 *
 * Description:
 * Use the given values to write a NETLINK header into the given buffer.
 *
 */
void nlbl_put_hdr(nlbl_data *buffer,
		  const unsigned int msg_type,
		  const unsigned short msg_len,
		  const unsigned short msg_flags,
		  const unsigned int msg_pid,
		  const unsigned int msg_seq)
{
  struct nlmsghdr *hdr = (struct nlmsghdr *)buffer;
  hdr->nlmsg_len = msg_len;
  hdr->nlmsg_type = msg_type;
  hdr->nlmsg_flags = msg_flags;
  hdr->nlmsg_seq = msg_seq;
  hdr->nlmsg_pid = msg_pid;
}

/**
 * nlbl_put_genlhdr - Write a Generic NETLINK header into a buffer
 * @buffer: the buffer
 * @cmd: the command
 *
 * Description:
 * Use the given values to write a Generic NETLINK header into the given
 * buffer.
 *
 */
void nlbl_put_genlhdr(nlbl_data *buffer, unsigned int cmd)
{
  struct genlmsghdr *hdr = (struct genlmsghdr *)buffer;
  hdr->cmd = cmd;
  hdr->version = NETLBL_PROTO_VERSION;
  hdr->reserved = 0;
}

/**
 * nlbl_putinc_u8 - Write a u8 value into a buffer and increment the buffer
 * @buffer: the buffer
 * @val: the value
 * @rem_len: remaining length
 *
 * Description:
 * Write the value specified in @val into the buffer specified by @buffer
 * and advance the buffer pointer past the newly written value.  If @rem_len
 * is not NULL then decrement it by the field length.
 *
 */
void nlbl_putinc_u8(nlbl_data **buffer,
		    const unsigned char val,
		    ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;

  nla->nla_len = NLA_HDRLEN + sizeof(val);
  nla->nla_type = NLA_U8;

  *buffer += NLA_HDRLEN;
  *(unsigned char *)*buffer = val;
  *buffer += NLMSG_ALIGN(sizeof(val));

  if (rem_len != NULL)
    *rem_len -= NLA_HDRLEN + NLMSG_ALIGN(sizeof(val));
}

/**
 * nlbl_putinc_u16 - Write a u16 value into a buffer and increment the buffer
 * @buffer: the buffer
 * @val: the value
 * @rem_len: remaining length
 *
 * Description:
 * Write the value specified in @val into the buffer specified by @buffer
 * and advance the buffer pointer past the newly written value.  If @rem_len
 * is not NULL then decrement it by the field length.
 *
 */
void nlbl_putinc_u16(nlbl_data **buffer,
		     const unsigned short val,
		     ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;

  nla->nla_len = NLA_HDRLEN + sizeof(val);
  nla->nla_type = NLA_U16;

  *buffer += NLA_HDRLEN;
  *(unsigned short *)*buffer = val;
  *buffer += NLMSG_ALIGN(sizeof(val));

  if (rem_len != NULL)
    *rem_len -= NLA_HDRLEN + NLMSG_ALIGN(sizeof(val));
}

/**
 * nlbl_putinc_u32 - Write a u32 value into a buffer and increment the buffer
 * @buffer: the buffer
 * @val: the value
 * @rem_len: remaining length
 *
 * Description:
 * Write the value specified in @val into the buffer specified by @buffer
 * and advance the buffer pointer past the newly written value.  If @rem_len
 * is not NULL then decrement it by the field length.
 *
 */
void nlbl_putinc_u32(nlbl_data **buffer,
		     const unsigned int val,
		     ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;

  nla->nla_len = NLA_HDRLEN + sizeof(val);
  nla->nla_type = NLA_U32;

  *buffer += NLA_HDRLEN;
  *(unsigned int *)*buffer = val;
  *buffer += NLMSG_ALIGN(sizeof(val));

  if (rem_len != NULL)
    *rem_len -= NLA_HDRLEN + NLMSG_ALIGN(sizeof(val));
}

/**
 * nlbl_putinc_str - Write a string into a buffer and increment the buffer
 * @buffer: the buffer
 * @val: the value
 * @rem_len: remaining length
 *
 * Description:
 * Write the string specified in @val into the buffer specified by @buffer
 * and advance the buffer pointer past the newly written value.  If @rem_len
 * is not NULL then decrement it by the field length.
 *
 */
void nlbl_putinc_str(nlbl_data **buffer,
		     const char *val,
		     ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;

  nla->nla_len = NLA_HDRLEN + strlen(val) + 1;
  nla->nla_type = NLA_STRING;

  *buffer += NLA_HDRLEN;
  strcpy((char *)*buffer, val);
  *buffer += NLMSG_ALIGN(strlen(val) + 1);

  if (rem_len != NULL)
    *rem_len -= NLA_HDRLEN + NLMSG_ALIGN(strlen(val) + 1);
}

/**
 * nlbl_put_hdr - Write a NETLINK header into a buffer and increment the ptr
 * @buffer: the buffer
 * @msg_type: the NETLINK message type
 * @msg_len: the NETLINK message length
 * @msg_flags: the NETLINK message flags
 * @msg_pid: the NETLINK message PID
 * @msg_seq: the NETLINK message sequence number
 *
 * Description:
 * Use the given values to write a NETLINK header into the given buffer and
 * then increment the buffer pointer past the header.
 *
 */
void nlbl_putinc_hdr(nlbl_data **buffer,
		     const unsigned int msg_type,
		     const unsigned short msg_len,
		     const unsigned short msg_flags,
		     const unsigned int msg_pid,
		     const unsigned int msg_seq)
{
  nlbl_put_hdr(*buffer,
	       msg_type,
	       msg_len,
	       msg_flags,
	       msg_pid,
	       msg_seq);
  *buffer += NLMSG_HDRLEN;
}

/**
 * nlbl_putinc_genlhdr - Write a Generic NETLINK header into a buffer
 * @buffer: the buffer
 * @cmd: the command
 *
 * Description:
 * Use the given values to write a Generic NETLINK header into the given
 * buffer and then increment the buffer pointer past the header.
 *
 */
void nlbl_putinc_genlhdr(nlbl_data **buffer, unsigned int cmd)
{
  nlbl_put_genlhdr(*buffer, cmd);
  *buffer += GENL_HDRLEN;
}

/**
 * nlbl_get_len - Return the data length of the current field
 * @buffer: the buffer
 *
 * Description:
 * Returns the length, in bytes, of the current NetLabel field not including
 * any padding.
 *
 */
size_t nlbl_get_len(const nlbl_data *buffer)
{
  struct nlattr *nla = (struct nlattr *)buffer;
  return nla->nla_len - NLA_HDRLEN;
}

/**
 * nlbl_get_payload - Return a pointer to the payload
 * @buffer: the buffer
 *
 * Description:
 * Return a pointer to the buffer's payload.
 *
 */
void *nlbl_get_payload(const nlbl_data *buffer)
{
  return (void *)(buffer + NLA_HDRLEN);
}

/**
 * nlbl_get_u8 - Read a u8 value from a buffer
 * @buffer: the buffer
 *
 * Description:
 * Return a u8 value pointed to by @buffer.
 *
 */
unsigned char nlbl_get_u8(const nlbl_data *buffer)
{
  return *(unsigned char *)nlbl_get_payload(buffer);
}

/**
 * nlbl_get_u16 - Read a u16 value from a buffer
 * @buffer: the buffer
 *
 * Description:
 * Return a u16 value pointed to by @buffer.
 *
 */
unsigned short nlbl_get_u16(const nlbl_data *buffer)
{
  return *(unsigned short *)nlbl_get_payload(buffer);
}

/**
 * nlbl_get_u32 - Read a u32 value from a buffer
 * @buffer: the buffer
 *
 * Description:
 * Return a u32 value pointed to by @buffer.
 *
 */
unsigned int nlbl_get_u32(const nlbl_data *buffer)
{
  return *(unsigned int *)nlbl_get_payload(buffer);
}

/**
 * nlbl_get_str - Read a string from a buffer
 * @buffer: the buffer
 *
 * Description:
 * Return a pointer to the string value pointed to by @buffer.
 *
 */
char *nlbl_get_str(const nlbl_data *buffer)
{
  size_t len = nlbl_get_len(buffer);
  char *str = malloc(len);
  if (str == NULL)
    return NULL;
  strncpy(str, nlbl_get_payload(buffer), len);
  return str;
}

/**
 * nlbl_getinc_u8 - Read a u8 value from a buffer and increment the buffer
 * @buffer: the buffer
 * @rem_len: remaining length
 *
 * Description:
 * Return a u8 value pointed to by @buffer and increment the buffer pointer
 * past the value.  If @rem_len is not NULL, decrement it by the field size.
 *
 */
unsigned char nlbl_getinc_u8(nlbl_data **buffer, ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;
  size_t len = NLMSG_ALIGN(nla->nla_len);
  unsigned char val = nlbl_get_u8(*buffer);
  *buffer += len;
  if (rem_len != NULL)
    *rem_len -= len;
  return val;
}

/**
 * nlbl_getinc_u16 - Read a u16 value from a buffer and increment the buffer
 * @buffer: the buffer
 * @rem_len: remaining length
 *
 * Description:
 * Return a u16 value pointed to by @buffer and increment the buffer pointer
 * past the value.  If @rem_len is not NULL, decrement it by the field size.
 *
 */
unsigned short nlbl_getinc_u16(nlbl_data **buffer, ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;
  size_t len = NLMSG_ALIGN(nla->nla_len);
  unsigned short val = nlbl_get_u16(*buffer);
  *buffer += len;
  if (rem_len != NULL)
    *rem_len -= len;
  return val;
}

/**
 * nlbl_getinc_u32 - Read a u32 value from a buffer and increment the buffer
 * @buffer: the buffer
 * @rem_len: remaining length
 *
 * Description:
 * Return a u32 value pointed to by @buffer and increment the buffer pointer
 * past the value.  If @rem_len is not NULL, decrement it by the field size.
 *
 */
unsigned int nlbl_getinc_u32(nlbl_data **buffer, ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;
  size_t len = NLMSG_ALIGN(nla->nla_len);
  unsigned int val = nlbl_get_u32(*buffer);
  *buffer += len;
  if (rem_len != NULL)
    *rem_len -= len;
  return val;
}

/**
 * nlbl_getinc_str - Read a string from a buffer and increment the buffer
 * @buffer: the buffer
 * @rem_len: remaining length
 *
 * Description:
 * Return a string pointer pointed to by @buffer and increment the buffer
 * pointer past the value.  If @rem_len is not NULL, decrement it by the field
 * size.
 *
 */
char *nlbl_getinc_str(nlbl_data **buffer, ssize_t *rem_len)
{
  struct nlattr *nla = (struct nlattr *)*buffer;
  size_t len = NLMSG_ALIGN(nla->nla_len);
  char *val = nlbl_get_str(*buffer);
  *buffer += len;
  if (rem_len != NULL)
    *rem_len -= len;
  return val;
}

/**
 * nlbl_len_payload - Return the size of a NetLabel field
 * @len: the length of the payload in bytes
 *
 * Description:
 * Returns the size in bytes of a NetLabel field given a payload of
 * length @len.
 *
 */
size_t nlbl_len_payload(size_t len)
{
  return NLA_HDRLEN + NLMSG_ALIGN(len);
}
