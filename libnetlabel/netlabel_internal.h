/*
 * NetLabel Library Internals
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

#ifndef _NETLINK_COMM_H
#define _NETLINK_COMM_H_

#include <netlink/netlink.h>

/* NetLabel communication handle */
struct nlbl_handle_s {
  struct nl_handle *nl_hndl;
};

/* Specify which version of libnl we are using */
/*  1.0-pre5 => 1005 */
/*  1.0-pre6 => 1006 */
#define LIBNL_VERSION           1005

/* XXX - this whole block will most likely go away once libnl supports Generic
 * Netlink */
#if 1 /* Generic Netlink types */

/* Generic Netlink message header */
struct genlmsghdr {
  uint8_t cmd;
  uint8_t version;
  uint16_t reserved;
};

#define GENL_ID_CTRL            0x10

enum {
  CTRL_CMD_UNSPEC,
  CTRL_CMD_NEWFAMILY,
  CTRL_CMD_DELFAMILY,
  CTRL_CMD_GETFAMILY,
  CTRL_CMD_NEWOPS,
  CTRL_CMD_DELOPS,
  CTRL_CMD_GETOPS,
  __CTRL_CMD_MAX,
};
#define CTRL_CMD_MAX (__CTRL_CMD_MAX - 1)

enum {
  CTRL_ATTR_UNSPEC,
  CTRL_ATTR_FAMILY_ID,
  CTRL_ATTR_FAMILY_NAME,
  __CTRL_ATTR_MAX,
};
#define CTRL_ATTR_MAX (__CTRL_ATTR_MAX - 1)

#endif /* Generic Netlink types */

#endif
