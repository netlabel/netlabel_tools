/*
 * Header file for libnetlabel.a
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

#define NETLBL_VER_STRING               VERSION_LIBNETLABEL

/*
 * Types
 */

/* general types */

/**
 * struct nlbl_handle - NetLabel communications handle
 *
 * Description:
 * XXX
 *
 */
struct nlbl_handle;

/**
 * nlbl_msg - NetLabel message
 *
 * Description:
 * XXX
 *
 */
typedef struct nl_msg nlbl_msg;

/**
 * nlbl_cmd - NetLabel command
 *
 * Description:
 * XXX
 *
 */
typedef uint16_t nlbl_cmd;

/**
 * nlbl_proto - NetLabel labeling protocol
 *
 * Description:
 * XXX
 *
 */
typedef uint32_t nlbl_proto;

/**
 * nlbl_netdev - NetLabel network device
 *
 * Description:
 * XXX
 *
 */
typedef char *nlbl_netdev;

/**
 * struct nlbl_netaddr - NetLabel network address structure
 * @type: address family
 * @addr.v4: IPv4 address
 * @addr.v6: IPv6 address
 * @mask.v4: IPv4 address mask
 * @mask.v6: IPv6 address mask
 *
 * Description:
 * XXX
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
 * nlbl_secctx - NetLabel security label
 *
 * Description:
 * XXX
 *
 */
typedef char *nlbl_secctx;

/* CIPSOv4 types */

/**
 * nlbl_cv4_doi - NetLabel CIPSOv4 Domain Of Interpretation (DOI) value
 *
 * Description:
 * XXX
 *
 */
typedef uint32_t nlbl_cv4_doi;

/**
 * nlbl_cv4_mtype - NetLabel CIPSOv4 mapping type
 *
 * Description:
 * XXX
 *
 */
typedef uint32_t nlbl_cv4_mtype;

/**
 * nlbl_cv4_tag - NetLabel CIPSOv4 tag type
 *
 * Description:
 * XXX
 *
 */
typedef uint8_t nlbl_cv4_tag;

/**
 * struct nlbl_cv4_tag_a - NetLabel CIPSOv4 tag array
 * @array: array of tag types
 * @size: size of array
 *
 * Description:
 * XXX
 *
 */
struct nlbl_cv4_tag_a {
	nlbl_cv4_tag *array;
	size_t size;
};

/**
 * nlbl_cv4_lvl - NetLabel CIPSOv4 MLS level
 *
 * Description:
 * XXX
 *
 */
typedef uint32_t nlbl_cv4_lvl;

/**
 * struct nlbl_cv4_lvl_a - NetLabel CIPSOv4 MLS level array
 * @array: array of MLS levels
 * @size: size of array
 *
 * Description:
 * XXX
 *
 */
struct nlbl_cv4_lvl_a {
	nlbl_cv4_lvl *array;
	size_t size;
};

/**
 * nlbl_cv4_cat - NetLabel CIPSOv4 MLS category
 *
 * Description:
 * XXX
 *
 */
typedef uint32_t nlbl_cv4_cat;

/**
 * struct nlbl_cv4_cat_a - NetLabel CIPSOv4 MLS category array
 * @array: array of MLS categories
 * @size: size of array
 *
 * Description:
 * XXX
 *
 */
struct nlbl_cv4_cat_a {
	nlbl_cv4_cat *array;
	size_t size;
};

/* lsm/domain mapping types */

/**
 * struct nlbl_dommmap_addr - NetLabel LSM/Domain address selector structure
 * @addr: IP address
 * @proto_type: labeling protocol
 * @proto.cv4_doi: CIPSOv4 DOI
 * @next: next address selector
 *
 * Description:
 * XXX
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
 * struct nlbl_dommap - NetLabel LSM/Domain mapping structure
 * @domain: LSM domain
 * @proto_type: labeling protocol
 * @proto.cv4_doi: CIPSOv4 DOI
 * @proto.addrsel: IP address selector(s)
 *
 * Description:
 * XXX
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
 * struct nlbl_addrmap - NetLabel network address mapping structure
 * @dev: network device
 * @addr: network address
 * @label: security label
 *
 * Description:
 * XXX
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

/* init/exit */

int nlbl_init(void);
void nlbl_exit(void);

/* low-level communications */

/* control */
void nlbl_comm_timeout(uint32_t seconds);

/* netlabel handle i/o */
struct nlbl_handle *nlbl_comm_open(void);
int nlbl_comm_close(struct nlbl_handle *hndl);
int nlbl_comm_recv(struct nlbl_handle *hndl, nlbl_msg **msg);
int nlbl_comm_recv_raw(struct nlbl_handle *hndl, unsigned char **data);
int nlbl_comm_send(struct nlbl_handle *hndl, nlbl_msg *msg);

/* netlabel message handling */
nlbl_msg *nlbl_msg_new(void);
void nlbl_msg_free(nlbl_msg *msg);
struct nlmsghdr *nlbl_msg_nlhdr(nlbl_msg *msg);
struct genlmsghdr *nlbl_msg_genlhdr(nlbl_msg *msg);
struct nlmsgerr *nlbl_msg_err(nlbl_msg *msg);

/* netlabel attribute handling */
struct nlattr *nlbl_attr_head(nlbl_msg *msg);
struct nlattr *nlbl_attr_find(nlbl_msg *msg, int nla_type);

/* operations */

/* management */
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

/* unlabeled */
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

/* cipso/ipv4 */
int nlbl_cipsov4_add_std(struct nlbl_handle *hndl,
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
