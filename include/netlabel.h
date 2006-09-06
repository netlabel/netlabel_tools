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

/*
 * MGMT
 */

/* NetLabel Management commands */
enum {
	NLBL_MGMT_C_UNSPEC,
	NLBL_MGMT_C_ACK,
	NLBL_MGMT_C_ADD,
	NLBL_MGMT_C_REMOVE,
	NLBL_MGMT_C_LISTALL,
	NLBL_MGMT_C_ADDDEF,
	NLBL_MGMT_C_REMOVEDEF,
	NLBL_MGMT_C_LISTDEF,
	NLBL_MGMT_C_PROTOCOLS,
	NLBL_MGMT_C_VERSION,
	__NLBL_MGMT_C_MAX,
};
#define NLBL_MGMT_C_MAX (__NLBL_MGMT_C_MAX - 1)

/* NetLabel Management attributes */
enum {
	NLBL_MGMT_A_UNSPEC,
	NLBL_MGMT_A_SEQNUM,
	NLBL_MGMT_A_ERRNO,
	NLBL_MGMT_A_DOMAIN,
	NLBL_MGMT_A_PROTOCOL,
	NLBL_MGMT_A_VERSION,
	NLBL_MGMT_A_CV4DOI,
	__NLBL_MGMT_A_MAX,
};
#define NLBL_MGMT_A_MAX (__NLBL_MGMT_A_MAX - 1)

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

/* NetLabel CIPSOv4 attributes */
enum {
	NLBL_CIPSOV4_A_UNSPEC,
	NLBL_CIPSOV4_A_SEQNUM,
	NLBL_CIPSOV4_A_ERRNO,
	NLBL_CIPSOV4_A_DOI,
	NLBL_CIPSOV4_A_MTYPE,
	NLBL_CIPSOV4_A_TAGLST,
	NLBL_CIPSOV4_A_MLSLVLLST,
	NLBL_CIPSOV4_A_MLSCATLST,
	__NLBL_CIPSOV4_A_MAX,
};
#define NLBL_CIPSOV4_A_MAX (__NLBL_CIPSOV4_A_MAX - 1)

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

/* NetLabel Unlabeled attributes */
enum {
	NLBL_UNLABEL_A_UNSPEC,
	NLBL_UNLABEL_A_SEQNUM,
	NLBL_UNLABEL_A_ERRNO,
	NLBL_UNLABEL_A_ACPTFLG,
	__NLBL_UNLABEL_A_MAX,
};
#define NLBL_UNLABEL_A_MAX (__NLBL_UNLABEL_A_MAX - 1)

#endif /* _NETLABEL_H */
