/*
 * Domain/Protocol Mapping Functions
 *
 * Author: Paul Moore <paul.moore@hp.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006, 2008
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <libnetlabel.h>

#include "netlabelctl.h"

/**
 * Add a domain mapping to NetLabel
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Add the specified domain mapping to the NetLabel system.  Returns zero on
 * success, negative values on failure.
 *
 */
int map_add(int argc, char *argv[])
{
	int ret_val;
	uint32_t iter;
	char *mask;
	uint32_t mask_iter;
	uint8_t def_flag = 0;
	struct nlbl_dommap domain;
	struct nlbl_netaddr addr;
	char *domain_proto_extra = NULL;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	memset(&domain, 0, sizeof(domain));
	memset(&addr, 0, sizeof(addr));

	/* parse the arguments */
	iter = 0;
	while (iter < argc && argv[iter] != NULL) {
		if (strncmp(argv[iter], "domain:", 7) == 0) {
			domain.domain = argv[iter] + 7;
		} else if (strncmp(argv[iter], "address:", 8) == 0) {
			mask = strstr(argv[iter] + 8, "/");
			if (mask != NULL) {
				mask[0] = '\0';
				mask++;
			}
			ret_val = inet_pton(AF_INET,
					    argv[iter] + 8, &addr.addr.v4);
			if (ret_val > 0) {
				addr.type = AF_INET;
				mask_iter = (mask ? atoi(mask) : 32);
				for (; mask_iter > 0; mask_iter--) {
					addr.mask.v4.s_addr >>= 1;
					addr.mask.v4.s_addr |= 0x80000000;
				}
				addr.mask.v4.s_addr = htonl(addr.mask.v4.s_addr);
			} else {
				ret_val = inet_pton(AF_INET6,
						    argv[iter] + 8,
						    &addr.addr.v6);
				if (ret_val > 0) {
					uint32_t tmp_iter;
					addr.type = AF_INET6;
					mask_iter = (mask ? atoi(mask) : 128);
					for (tmp_iter = 0;
					     mask_iter > 0 && tmp_iter < 4;
					     tmp_iter++) {
						for (; mask_iter > 0 &&
							     addr.mask.v6.s6_addr32[tmp_iter] < 0xffffffff;
						     mask_iter--) {
							addr.mask.v6.s6_addr32[tmp_iter] >>= 1;
							addr.mask.v6.s6_addr32[tmp_iter] |= 0x80000000;
						}
						addr.mask.v6.s6_addr32[tmp_iter] =
							htonl(addr.mask.v6.s6_addr32[tmp_iter]);
					}
				}
			}
			if (ret_val <= 0)
				return -EINVAL;
		} else if (strncmp(argv[iter], "protocol:", 9) == 0) {
			/* protocol specifics */
			if (strncmp(argv[iter] + 9, "cipsov4", 7) == 0)
				domain.proto_type = NETLBL_NLTYPE_CIPSOV4;
			else if (strncmp(argv[iter] + 9, "unlbl", 5) == 0)
				domain.proto_type = NETLBL_NLTYPE_UNLABELED;
			else
				return -EINVAL;
			domain_proto_extra = strstr(argv[iter] + 9, ",");
			if (domain_proto_extra)
				domain_proto_extra++;
		} else if (strncmp(argv[iter], "default", 7) == 0) {
			def_flag = 1;
		} else
			return -EINVAL;
		iter++;
	}
	if (domain.domain == NULL && def_flag == 0)
		return -EINVAL;

	/* handle the protocol "extra" field */
	switch (domain.proto_type) {
	case NETLBL_NLTYPE_CIPSOV4:
		if (domain_proto_extra == NULL)
			return -EINVAL;
		domain.proto.cv4_doi = (nlbl_cv4_doi)atoi(domain_proto_extra);
		break;
	}

	/* add the mapping */
	if (def_flag)
		ret_val = nlbl_mgmt_adddef(NULL, &domain, &addr);
	else
		ret_val = nlbl_mgmt_add(NULL, &domain, &addr);

	return ret_val;
}

/**
 * Delete a domain mapping from NetLabel
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Description:
 * Remove the specified domain mapping from the NetLabel system.  Returns zero
 * on success, negative values on failure.
 *
 */
int map_del(int argc, char *argv[])
{
	int ret_val;
	uint32_t iter;
	uint32_t def_flag = 0;
	char *domain = NULL;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* parse the arguments */
	iter = 0;
	while (iter < argc && argv[iter] != NULL) {
		if (strncmp(argv[iter], "domain:", 7) == 0) {
			domain = argv[iter] + 7;
		} else if (strncmp(argv[iter], "default", 7) == 0) {
			def_flag = 1;
		} else
			return -EINVAL;
		iter++;
	}
	if (domain == NULL && def_flag == 0)
		return -EINVAL;

	/* remove the mapping */
	if (def_flag)
		ret_val = nlbl_mgmt_deldef(NULL);
	else
		ret_val = nlbl_mgmt_del(NULL, domain);

	return ret_val;
}

/**
 * List the NetLabel domains mappings
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * List the configured NetLabel domain mappings.  Returns zero on success,
 * negative values on failure.
 *
 */
int map_list(int argc, char *argv[])
{
	int ret_val;
	struct nlbl_dommap *domain_p = NULL;
	struct nlbl_dommap_addr *iter_addr;
	size_t count;
	uint32_t iter;

	/* get the list of mappings */
	ret_val = nlbl_mgmt_listall(NULL, &domain_p);
	if (ret_val < 0)
		goto list_return;
	count = ret_val;

	/* get the default mapping */
	domain_p = realloc(domain_p, sizeof(struct nlbl_dommap) * (count + 1));
	if (domain_p == NULL)
		goto list_return;
	memset(&domain_p[count], 0, sizeof(struct nlbl_dommap));
	ret_val = nlbl_mgmt_listdef(NULL, &domain_p[count]);
	if (ret_val < 0 && ret_val != -ENOENT)
		goto list_return;
	else if (ret_val == 0)
		count += 1;
	else
		ret_val = 0;

	/* display the results */
	if (opt_pretty) {
		printf("Configured NetLabel domain mappings (%zu)\n", count);
		for (iter = 0; iter < count; iter++) {
			/* domain string */
			printf(" domain: ");
			if (domain_p[iter].domain != NULL)
				printf("\"%s\"\n", domain_p[iter].domain);
			else
				printf("DEFAULT\n");
			/* protocol */
			switch (domain_p[iter].proto_type) {
			case NETLBL_NLTYPE_UNLABELED:
				printf("   protocol: UNLABELED\n");
				break;
			case NETLBL_NLTYPE_CIPSOV4:
				printf("   protocol: CIPSOv4, DOI = %u\n",
				       domain_p[iter].proto.cv4_doi);
				break;
			case NETLBL_NLTYPE_ADDRSELECT:
				iter_addr = domain_p[iter].proto.addrsel;
				while (iter_addr) {
					printf("   address: ");
					nlctl_addr_print(&iter_addr->addr);
					printf("\n"
					       "    protocol: ");
					switch (iter_addr->proto_type) {
					case NETLBL_NLTYPE_UNLABELED:
						printf("UNLABELED\n");
						break;
					case NETLBL_NLTYPE_CIPSOV4:
						printf("CIPSOv4, DOI = %u\n",
						       iter_addr->proto.cv4_doi);
						break;
					default:
						printf("UNKNOWN(%u)\n",
						       iter_addr->proto_type);
						break;
					}
					iter_addr = iter_addr->next;
				}
				break;
			default:
				printf("UNKNOWN(%u)\n",
				       domain_p[iter].proto_type);
				break;
			}
		}
	} else {
		for (iter = 0; iter < count; iter++) {
			/* domain string */
			printf("domain:");
			if (domain_p[iter].domain != NULL)
				printf("\"%s\",", domain_p[iter].domain);
			else
				printf("DEFAULT,");
			/* protocol */
			switch (domain_p[iter].proto_type) {
			case NETLBL_NLTYPE_UNLABELED:
				printf("UNLABELED");
				break;
			case NETLBL_NLTYPE_CIPSOV4:
				printf("CIPSOv4,%u",
				       domain_p[iter].proto.cv4_doi);
				break;
			case NETLBL_NLTYPE_ADDRSELECT:
				iter_addr = domain_p[iter].proto.addrsel;
				while (iter_addr) {
					printf("address:");
					nlctl_addr_print(&iter_addr->addr);
					printf(",protocol:");
					switch (iter_addr->proto_type) {
					case NETLBL_NLTYPE_UNLABELED:
						printf("UNLABELED");
						break;
					case NETLBL_NLTYPE_CIPSOV4:
						printf("CIPSOv4,%u",
						       iter_addr->proto.cv4_doi);
						break;
					default:
						printf("UNKNOWN(%u)",
						       iter_addr->proto_type);
						break;
					}
					iter_addr = iter_addr->next;
				}
				break;
			default:
				printf("UNKNOWN(%u)",
				       domain_p[iter].proto_type);
				break;
			}
			if (iter + 1 < count)
				printf(" ");
		}
		printf("\n");
	}

list_return:
	if (domain_p) {
		for (iter = 0; iter < count; iter++)
			if (domain_p[iter].domain)
				free(domain_p[iter].domain);
		free(domain_p);
	}
	return ret_val;
}

/**
 * Entry point for the NetLabel mapping functions
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Parses the argument list and performs the requested operation.  Returns zero
 * on success, negative values on failure.
 *
 */
int map_main(int argc, char *argv[])
{
	int ret_val;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* handle the request */
	if (strcmp(argv[0], "add") == 0) {
		/* add a domain mapping */
		ret_val = map_add(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "del") == 0) {
		/* delete a domain mapping */
		ret_val = map_del(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "list") == 0) {
		/* list the domain mappings */
		ret_val = map_list(argc - 1, argv + 1);
	} else {
		/* unknown request */
		ret_val = -EINVAL;
	}

	return ret_val;
}
