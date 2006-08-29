/*
 * Header file for libnetlabel.a
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

#ifndef _LIBNETLABEL_H
#define _LIBNETLABEL_H

/*
 * Version
 */
#define NETLBL_VER_STRING               "0.16"
#define NETLBL_VER_DATE                 "August 3, 2006"

/*
 * Types
 */

/* generic types */
typedef int nlbl_socket;
typedef unsigned int nlbl_type;
typedef unsigned char nlbl_data;

/* cipso/ipv4 types */
typedef unsigned int cv4_doi;
typedef unsigned int cv4_maptype;
typedef unsigned char cv4_tag;
typedef struct cv4_tag_array_s {
  cv4_tag *array;
  ssize_t size;
} cv4_tag_array;
typedef unsigned int cv4_lvl;
typedef struct cv4_lvl_array_s {
  cv4_lvl *array;
  ssize_t size;
} cv4_lvl_array;
typedef unsigned int cv4_cat;
typedef struct cv4_cat_array_s {
  cv4_cat *array;
  ssize_t size;
} cv4_cat_array;

/* management types */
typedef struct mgmt_domain_s {
  char *domain;
  size_t domain_len;
  nlbl_type proto_type;
  union {
    struct {
      cv4_doi doi;
      cv4_maptype maptype;
    } cipsov4;
  } proto;
} mgmt_domain;

/*
 * Functions
 */

/*** init/exit */
int nlbl_netlink_init(void);
void nlbl_netlink_exit(void);

/*** field functions */
#define NETLBL_LEN_U8                   nlbl_len_payload(1)
#define NETLBL_LEN_U16                  nlbl_len_payload(2)
#define NETLBL_LEN_U32                  nlbl_len_payload(4)
size_t nlbl_len_payload(size_t len);
void nlbl_putinc_u8(nlbl_data **buffer,
		    const unsigned char val,
		    ssize_t *rem_len);
void nlbl_putinc_u16(nlbl_data **buffer,
		     const unsigned short val,
		     ssize_t *rem_len);
void nlbl_putinc_u32(nlbl_data **buffer,
		     const unsigned int val,
		     ssize_t *rem_len);
void nlbl_putinc_str(nlbl_data **buffer,
		     const char *val,
		     ssize_t *rem_len);
size_t nlbl_get_len(const nlbl_data *buffer);
void *nlbl_get_payload(const nlbl_data *buffer);
unsigned char nlbl_get_u8(const nlbl_data *buffer);
unsigned short nlbl_get_u16(const nlbl_data *buffer);
unsigned int nlbl_get_u32(const nlbl_data *buffer);
char *nlbl_get_str(const nlbl_data *buffer);
unsigned char nlbl_getinc_u8(nlbl_data **buffer, ssize_t *rem_len);
unsigned short nlbl_getinc_u16(nlbl_data **buffer, ssize_t *rem_len);
unsigned int nlbl_getinc_u32(nlbl_data **buffer, ssize_t *rem_len);
char *nlbl_getinc_str(nlbl_data **buffer, ssize_t *rem_len);

/*** low-level communications */

/* utility functions */
void nlbl_netlink_timeout(unsigned int seconds);
size_t nlbl_netlink_msgspace(size_t msg_len);

/* raw netlink */
int nlbl_netlink_open(nlbl_socket *sock);
int nlbl_netlink_close(nlbl_socket sock);
int nlbl_netlink_read(nlbl_socket sock,
                      nlbl_type *type,
                      nlbl_data **msg,
                      ssize_t *msg_len);
int nlbl_netlink_write(nlbl_socket sock, 
                       nlbl_type type, 
                       nlbl_data *msg, 
                       size_t msg_len);

/* management */
int nlbl_mgmt_write(nlbl_socket sock, nlbl_data *msg, size_t msg_len);

/* unlabeled */
int nlbl_unlabeled_write(nlbl_socket sock, nlbl_data *msg, size_t msg_len);

/* cipso/ipv4 */
int nlbl_cipsov4_write(nlbl_socket sock, nlbl_data *msg, size_t msg_len);

/*** operations */

/* management */
int nlbl_mgmt_modules(nlbl_socket sock,
                      unsigned int **module_list,
                      size_t *module_count);
int nlbl_mgmt_version(nlbl_socket sock, unsigned int *version);
int nlbl_mgmt_add(nlbl_socket sock,
                  mgmt_domain *domain_list,
                  size_t domain_count,
                  unsigned int def_flag);
int nlbl_mgmt_del(nlbl_socket sock,
                  char **domains,
                  size_t *domain_len,
                  size_t domain_count,
                  unsigned int def_flag);
int nlbl_mgmt_list(nlbl_socket sock, 
                   mgmt_domain **domain_list,
                   size_t *domain_count,
                   unsigned int def_flag);

/* unlabeled */
int nlbl_unlabeled_accept(nlbl_socket sock, unsigned int allow_flag);
int nlbl_unlabeled_list(nlbl_socket sock, unsigned int *allow_flag);

/* cipso/ipv4 */
int nlbl_cipsov4_add_std(nlbl_socket sock,
                         cv4_doi doi,
                         cv4_tag_array *tags,
                         cv4_lvl_array *lvls,
                         cv4_cat_array *cats);
int nlbl_cipsov4_add_pass(nlbl_socket sock,
			  cv4_doi doi,
			  cv4_tag_array *tags);
int nlbl_cipsov4_del(nlbl_socket sock, cv4_doi doi);
int nlbl_cipsov4_list(nlbl_socket sock,
                      cv4_doi doi,
		      cv4_maptype *maptype,
                      cv4_tag_array *tags,
                      cv4_lvl_array *lvls,
                      cv4_cat_array *cats);
int nlbl_cipsov4_list_all(nlbl_socket sock, 
                          cv4_doi **doi_list,
                          cv4_maptype **mtype_list,
                          size_t *count);

#endif
