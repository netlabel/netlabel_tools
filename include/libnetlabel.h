/** @file
 * NetLabel userspace configuration library API.
 *
 * The Linux NetLabel subsystem manages network security labels for explicit
 * labeling protocols such as CIPSO as well as static security labels for
 * "unlabeled" network traffic.  More information on NetLabel can be found at
 * the NetLabel SourceForge project site, http://netlabel.sf.net.
 *
 * Author: Paul Moore <paul@paul-moore.com>
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

#ifndef _LIBNETLABEL_H
#define _LIBNETLABEL_H

#include <sys/types.h>
#include <linux/types.h>
#include <netinet/in.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include <netlabel.h>
#include <version.h>

/*
 * Version
 */

#define NETLBL_VER_STRING		VERSION_RELEASE

/*
 * Types
 */

/* General Types */

/**
 * NetLabel communications handle
 *
 * Handle used for communicating with the NetLabel subsystem in the kernel via
 * generic netlink.
 *
 */
struct nlbl_handle;

/**
 * NetLabel message
 *
 * NetLabel type used for sending and receiving messages with the NetLabel
 * kernel subsystem.
 *
 */
typedef struct nl_msg nlbl_msg;

/**
 * NetLabel labeling protocol
 *
 * NetLabel type used to specify network labeling protocols.
 *
 */
typedef uint32_t nlbl_proto;

/**
 * NetLabel network device
 *
 * NetLabel type used to specify network interfaces.
 *
 */
typedef char *nlbl_netdev;

/**
 * NetLabel network address structure
 * @param type address family
 * @param addr.v4 IPv4 address
 * @param addr.v6 IPv6 address
 * @param mask.v4 IPv4 address mask
 * @param mask.v6 IPv6 address mask
 *
 * NetLabel type used to represent IP addresses.  It can represent both single
 * hosts and entire networks using both IPv4 and IPv6.
 *
 */
struct nlbl_netaddr {
	short type;
	union {
		struct in_addr v4;
		struct in6_addr v6;
	} addr;
	union {
		struct in_addr v4;
		struct in6_addr v6;
	} mask;
};

/**
 * NetLabel security label
 *
 * NetLabel type used to represent security labels.  NetLabel itself does not
 * interpret the security labels, the individual LSMs are used to parse and
 * interpret the security labels.
 *
 */
typedef char *nlbl_secctx;

/* CIPSOv4 Types */

/**
 * NetLabel CIPSOv4 Domain Of Interpretation (DOI) value
 *
 * NetLabel type used to represent a CIPSO Domian of Interpretation (DOI).
 *
 */
typedef uint32_t nlbl_cv4_doi;

/**
 * NetLabel CIPSOv4 mapping type
 *
 * NetLabel type used to represent the CIPSOv4 security label mapping method.
 *
 */
typedef uint32_t nlbl_cv4_mtype;

/**
 * NetLabel CIPSOv4 tag type
 *
 * NetLabel type used to represent CIPSOv4 tag types.
 *
 */
typedef uint8_t nlbl_cv4_tag;

/**
 * NetLabel CIPSOv4 tag array
 * @param array array of tag types
 * @param size size of array
 *
 * NetLabel type used to represent an array of CIPSOv4 tags in decreasing order
 * of preference.
 *
 */
struct nlbl_cv4_tag_a {
	nlbl_cv4_tag *array;
	size_t size;
};

/**
 * NetLabel CIPSOv4 MLS level
 *
 * NetLabel type used to represent the CIPSOv4 MLS sensitivity level.
 *
 */
typedef uint32_t nlbl_cv4_lvl;

/**
 * NetLabel CIPSOv4 MLS level array
 * @param array array of MLS levels
 * @param size size of array
 *
 * NetLabel type used to represent an array of CIPSOv4 MLS sensitivity levels.
 *
 */
struct nlbl_cv4_lvl_a {
	nlbl_cv4_lvl *array;
	size_t size;
};

/**
 * NetLabel CIPSOv4 MLS category
 *
 * NetLabel type used to represent the CIPSOv4 MLS category/compartment.
 *
 */
typedef uint32_t nlbl_cv4_cat;

/**
 * NetLabel CIPSOv4 MLS category array
 * @param array array of MLS categories
 * @param size size of array
 *
 * NetLabel type used to represent an array of CIPSOv4 MLS categories.
 *
 */
struct nlbl_cv4_cat_a {
	nlbl_cv4_cat *array;
	size_t size;
};

/* NetLabel and LSM Mapping Types */

/**
 * NetLabel IP address selector structure
 * @param addr IP address
 * @param proto_type labeling protocol
 * @param proto.cv4_doi CIPSOv4 DOI
 * @param next next address selector
 *
 * NetLabel type used to map IP addresses to labeling protocol configurations.
 *
 */
struct nlbl_dommap_addr {
	struct nlbl_netaddr addr;
	nlbl_proto proto_type;
	union {
		nlbl_cv4_doi cv4_doi;
	} proto;

	struct nlbl_dommap_addr *next;
};

/**
 * NetLabel LSM/Domain mapping structure
 * @param domain LSM domain
 * @param proto_type labeling protocol
 * @param proto.cv4_doi CIPSOv4 DOI
 * @param proto.addrsel IP address selector(s)
 *
 * NetLabel type used to map LSM domains to labeling protocol configurations.
 *
 */
struct nlbl_dommap {
	char *domain;
	nlbl_proto proto_type;
	union {
		nlbl_cv4_doi cv4_doi;
		struct nlbl_dommap_addr *addrsel;
	} proto;
};

/**
 * NetLabel network address mapping structure
 * @param dev network device
 * @param addr network address
 * @param label security label
 *
 * NetLabel type used to map network interfaces and addresses to security
 * labels.
 *
 */
struct nlbl_addrmap {
	nlbl_netdev dev;
	struct nlbl_netaddr addr;
	nlbl_secctx label;
};

/*
 * Functions
 */

/* Initialization and Termination */

int nlbl_init(void);
void nlbl_exit(void);

/* Low Level Communications */

/* Communications Control */
void nlbl_comm_timeout(uint32_t seconds);

/* Raw NetLabel I/O API */
struct nlbl_handle *nlbl_comm_open(void);
int nlbl_comm_close(struct nlbl_handle *hndl);
int nlbl_comm_recv(struct nlbl_handle *hndl, nlbl_msg **msg);
int nlbl_comm_recv_raw(struct nlbl_handle *hndl, unsigned char **data);
int nlbl_comm_send(struct nlbl_handle *hndl, nlbl_msg *msg);

/* Message Handling */
nlbl_msg *nlbl_msg_new(void);
void nlbl_msg_free(nlbl_msg *msg);
struct nlmsghdr *nlbl_msg_nlhdr(nlbl_msg *msg);
struct genlmsghdr *nlbl_msg_genlhdr(nlbl_msg *msg);
struct nlmsgerr *nlbl_msg_err(nlbl_msg *msg);

/* Attribute Handling */
struct nlattr *nlbl_attr_head(nlbl_msg *msg);
struct nlattr *nlbl_attr_find(nlbl_msg *msg, int nla_type);

/* Configuration Operations */

/* Management */
int nlbl_mgmt_version(struct nlbl_handle *hndl, uint32_t *version);
int nlbl_mgmt_protocols(struct nlbl_handle *hndl, nlbl_proto **protocols);
int nlbl_mgmt_add(struct nlbl_handle *hndl,
		  struct nlbl_dommap *domain,
		  struct nlbl_netaddr *addr);
int nlbl_mgmt_adddef(struct nlbl_handle *hndl,
		     struct nlbl_dommap *domain,
		     struct nlbl_netaddr *addr);
int nlbl_mgmt_del(struct nlbl_handle *hndl, char *domain);
int nlbl_mgmt_deldef(struct nlbl_handle *hndl);
int nlbl_mgmt_listall(struct nlbl_handle *hndl, struct nlbl_dommap **domains);
int nlbl_mgmt_listdef(struct nlbl_handle *hndl, struct nlbl_dommap *domain);

/* Unlabeled Traffic */
int nlbl_unlbl_accept(struct nlbl_handle *hndl, uint8_t allow_flag);
int nlbl_unlbl_list(struct nlbl_handle *hndl, uint8_t *allow_flag);
int nlbl_unlbl_staticadd(struct nlbl_handle *hndl,
			 nlbl_netdev dev,
			 struct nlbl_netaddr *addr,
			 nlbl_secctx label);
int nlbl_unlbl_staticadddef(struct nlbl_handle *hndl,
			    struct nlbl_netaddr *addr,
			    nlbl_secctx label);
int nlbl_unlbl_staticdel(struct nlbl_handle *hndl,
			 nlbl_netdev dev,
			 struct nlbl_netaddr *addr);
int nlbl_unlbl_staticdeldef(struct nlbl_handle *hndl,
			    struct nlbl_netaddr *addr);
int nlbl_unlbl_staticlist(struct nlbl_handle *hndl,
			  struct nlbl_addrmap **addrs);
int nlbl_unlbl_staticlistdef(struct nlbl_handle *hndl,
			     struct nlbl_addrmap **addrs);

/* CIPSOv4 Protocol */
int nlbl_cipsov4_add_trans(struct nlbl_handle *hndl,
			   nlbl_cv4_doi doi,
			   struct nlbl_cv4_tag_a *tags,
			   struct nlbl_cv4_lvl_a *lvls,
			   struct nlbl_cv4_cat_a *cats);
int nlbl_cipsov4_add_pass(struct nlbl_handle *hndl,
			  nlbl_cv4_doi doi,
			  struct nlbl_cv4_tag_a *tags);
int nlbl_cipsov4_add_local(struct nlbl_handle *hndl, nlbl_cv4_doi doi);
int nlbl_cipsov4_del(struct nlbl_handle *hndl, nlbl_cv4_doi doi);
int nlbl_cipsov4_list(struct nlbl_handle *hndl,
		      nlbl_cv4_doi doi,
		      nlbl_cv4_mtype *mtype,
		      struct nlbl_cv4_tag_a *tags,
		      struct nlbl_cv4_lvl_a *lvls,
		      struct nlbl_cv4_cat_a *cats);
int nlbl_cipsov4_listall(struct nlbl_handle *hndl,
			 nlbl_cv4_doi **dois,
			 nlbl_cv4_mtype **mtypes);

#endif
