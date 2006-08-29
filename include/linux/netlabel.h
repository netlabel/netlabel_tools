/*
 * NetLabel System
 *
 * The NetLabel system manages static and dynamic label mappings for network 
 * protocols such as CIPSO and RIPSO.
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

#ifndef _NETLABEL_H
#define _NETLABEL_H

/*
 * Generic NETLINK interface
 */

#define INCLUDE_NL_GUTS
#ifdef INCLUDE_NL_GUTS

/* FIXME: perhaps some or all this lives in a system header file? */

#define NLMSG_HDRLEN            NLMSG_ALIGN(sizeof(struct nlmsghdr))

struct nlattr
{
  unsigned short nla_len;
  unsigned short nla_type;
};

enum {
  NLA_UNSPEC,
  NLA_U8,
  NLA_U16,
  NLA_U32,
  NLA_U64,
  NLA_STRING,
  NLA_FLAG,
  NLA_MSECS,
  NLA_NESTED,
  __NLA_TYPE_MAX,
};
#define NLA_TYPE_MAX (__NLA_TYPE_MAX - 1)

#define NLA_HDRLEN              NLMSG_ALIGN(sizeof(struct nlattr))

#define NETLINK_GENERIC		16

struct genlmsghdr {
  unsigned char cmd;
  unsigned char version;
  unsigned short reserved;
};
#define GENL_HDRLEN	        NLMSG_ALIGN(sizeof(struct genlmsghdr))

#define GENL_ID_GENERATE	0
#define GENL_ID_CTRL		0x10

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

#endif /* INCLUDE_NL_GUTS */

/*
 * NetLabel NETLINK protocol
 */

#define NETLBL_PROTO_VERSION            1

/* NetLabel NETLINK types/families */
#define NETLBL_NLTYPE_NONE              0
#define NETLBL_NLTYPE_MGMT              1
#define NETLBL_NLTYPE_MGMT_NAME         "NLBL_MGMT"
#define NETLBL_NLTYPE_RIPSO             2
#define NETLBL_NLTYPE_RIPSO_NAME        "NLBL_RIPSO"
#define NETLBL_NLTYPE_CIPSOV4           3
#define NETLBL_NLTYPE_CIPSOV4_NAME      "NLBL_CIPSOv4"
#define NETLBL_NLTYPE_CIPSOV6           4
#define NETLBL_NLTYPE_CIPSOV6_NAME      "NLBL_CIPSOv6"
#define NETLBL_NLTYPE_UNLABELED         5
#define NETLBL_NLTYPE_UNLABELED_NAME    "NLBL_UNLBL"

/* Generic return codes */
#define NETLBL_E_OK                     0

/*
 * MGMT
 */

/* NetLabel Management commands */
enum {
  NLBL_MGMT_C_UNSPEC,
  NLBL_MGMT_C_ACK,
  NLBL_MGMT_C_ADD,
  NLBL_MGMT_C_REMOVE,
  NLBL_MGMT_C_LIST,
  NLBL_MGMT_C_ADDDEF,
  NLBL_MGMT_C_REMOVEDEF,
  NLBL_MGMT_C_LISTDEF,
  NLBL_MGMT_C_MODULES,
  NLBL_MGMT_C_VERSION,
  __NLBL_MGMT_C_MAX,
};
#define NLBL_MGMT_C_MAX (__NLBL_MGMT_C_MAX - 1)

/*
 * RIPSO
 */

/* XXX - TBD */

/*
 * CIPSO V4
 */

/* CIPSOv4 DOI map types */
#define CIPSO_V4_MAP_UNKNOWN          0
#define CIPSO_V4_MAP_STD              1
#define CIPSO_V4_MAP_PASS             2

/* NetLabel CIPSOv4 commands */
enum {
  NLBL_CIPSOV4_C_UNSPEC,
  NLBL_CIPSOV4_C_ACK,
  NLBL_CIPSOV4_C_ADD,
  NLBL_CIPSOV4_C_REMOVE,
  NLBL_CIPSOV4_C_LIST,
  NLBL_CIPSOV4_C_LISTALL,
  __NLBL_CIPSOV4_C_MAX,
};
#define NLBL_CIPSOV4_C_MAX (__NLBL_CIPSOV4_C_MAX - 1)

/*
 * CIPSO V6
 */

/* XXX - TBD */

/*
 * UNLABELED
 */

/* NetLabel Unlabeled commands */
enum {
  NLBL_UNLABEL_C_UNSPEC,
  NLBL_UNLABEL_C_ACK,
  NLBL_UNLABEL_C_ACCEPT,
  NLBL_UNLABEL_C_LIST,
  __NLBL_UNLABEL_C_MAX,
};
#define NLBL_UNLABEL_C_MAX (__NLBL_UNLABEL_C_MAX - 1)

#endif /* _NETLABEL_H */
