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

/*** communication types */

typedef struct nlbl_handle_s nlbl_handle;
typedef struct nl_msg nlbl_msg;
typedef uint16_t nlbl_cmd;

/*** protocol type */

typedef uint32_t nlbl_proto;

/*** cipso/ipv4 types */

/* domain of interpretation (doi) */
typedef uint32_t nlbl_cv4_doi;

typedef uint32_t nlbl_cv4_mtype;

/* tags */
typedef uint8_t nlbl_cv4_tag;
typedef struct nlbl_cv4_tag_array_s {
  nlbl_cv4_tag *array;
  size_t size;
} nlbl_cv4_tag_a;

/* mls sensitivity levels */
typedef uint32_t nlbl_cv4_lvl;
typedef struct nlbl_cv4_lvl_array_s {
  nlbl_cv4_lvl *array;
  size_t size;
} nlbl_cv4_lvl_a;

/* mls categories */
typedef uint32_t nlbl_cv4_cat;
typedef struct cv4_cat_array_s {
  nlbl_cv4_cat *array;
  size_t size;
} nlbl_cv4_cat_a;

/*** management types */

typedef struct nlbl_mgmt_domain_s {
  char *domain;
  nlbl_proto proto_type;
  union {
    struct {
      nlbl_cv4_doi doi;
    } cv4;
  } proto;
} nlbl_mgmt_domain;

/*
 * Functions
 */

/*** init/exit */

int nlbl_init(void);
void nlbl_exit(void);

/*** low-level communications */

/* control */
void nlbl_comm_timeout(uint32_t seconds);

/* netlabel handle i/o */
nlbl_handle *nlbl_comm_open(void);
int nlbl_comm_close(nlbl_handle *hndl);
int nlbl_comm_recv(nlbl_handle *hndl, nlbl_msg **msg);
int nlbl_comm_recv_raw(nlbl_handle *hndl, unsigned char **data);
int nlbl_comm_send(nlbl_handle *hndl, nlbl_msg *msg);

/* netlabel message handling */
nlbl_msg *nlbl_msg_new(void);
void nlbl_msg_free(nlbl_msg *msg);
struct nlmsghdr *nlbl_msg_nlhdr(nlbl_msg *msg);
struct genlmsghdr *nlbl_msg_genlhdr(nlbl_msg *msg);
struct nlmsgerr *nlbl_msg_err(nlbl_msg *msg);

/* netlabel attribute handling */
struct nlattr *nlbl_attr_head(nlbl_msg *msg);
struct nlattr *nlbl_attr_find(nlbl_msg *msg, int nla_type);

/*** operations */

/* management */
int nlbl_mgmt_version(nlbl_handle *hndl, uint32_t *version);
int nlbl_mgmt_protocols(nlbl_handle *hndl, nlbl_proto **protocols);
int nlbl_mgmt_add(nlbl_handle *hndl, nlbl_mgmt_domain *domain);
int nlbl_mgmt_adddef(nlbl_handle *hndl, nlbl_mgmt_domain *domain);
int nlbl_mgmt_del(nlbl_handle *hndl, char *domain);
int nlbl_mgmt_deldef(nlbl_handle *hndl);
int nlbl_mgmt_listall(nlbl_handle *hndl, nlbl_mgmt_domain **domains);
int nlbl_mgmt_listdef(nlbl_handle *hndl, nlbl_mgmt_domain *domain);

/* unlabeled */
int nlbl_unlbl_accept(nlbl_handle *hndl, uint8_t allow_flag);
int nlbl_unlbl_list(nlbl_handle *hndl, uint8_t *allow_flag);

/* cipso/ipv4 */
int nlbl_cipsov4_add_std(nlbl_handle *hndl,
                         nlbl_cv4_doi doi,
                         nlbl_cv4_tag_a *tags,
                         nlbl_cv4_lvl_a *lvls,
                         nlbl_cv4_cat_a *cats);
int nlbl_cipsov4_add_pass(nlbl_handle *hndl,
			  nlbl_cv4_doi doi,
			  nlbl_cv4_tag_a *tags);
int nlbl_cipsov4_del(nlbl_handle *hndl, nlbl_cv4_doi doi);
int nlbl_cipsov4_list(nlbl_handle *hndl,
                      nlbl_cv4_doi doi,
		      nlbl_cv4_mtype *mtype,
                      nlbl_cv4_tag_a *tags,
                      nlbl_cv4_lvl_a *lvls,
                      nlbl_cv4_cat_a *cats);
int nlbl_cipsov4_listall(nlbl_handle *hndl,
			 nlbl_cv4_doi **dois,
			 nlbl_cv4_mtype **mtypes);

#endif
