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
	uint32_t iter;
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
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strncmp(argv[iter], "domain:", 7) == 0) {
			domain.domain = argv[iter] + 7;
		} else if (strncmp(argv[iter], "address:", 8) == 0) {
			if (nlctl_addr_parse(argv[iter] + 8, &addr) != 0)
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
	}

	/* handle the protocol "extra" field */
	switch (domain.proto_type) {
	case NETLBL_NLTYPE_CIPSOV4:
		if (domain_proto_extra == NULL)
			return -EINVAL;
		domain.proto.cv4_doi = atoi(domain_proto_extra);
		break;
	}

	/* add the mapping */
	if (def_flag != 0)
		return nlbl_mgmt_adddef(NULL, &domain, &addr);
	else
		return nlbl_mgmt_add(NULL, &domain, &addr);
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
	uint32_t iter;
	uint32_t def_flag = 0;
	char *domain = NULL;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* parse the arguments */
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strncmp(argv[iter], "domain:", 7) == 0) {
			domain = argv[iter] + 7;
		} else if (strncmp(argv[iter], "default", 7) == 0) {
			def_flag = 1;
		} else
			return -EINVAL;
	}

	/* remove the mapping */
	if (def_flag != 0)
		return nlbl_mgmt_deldef(NULL);
	else
		return nlbl_mgmt_del(NULL, domain);
}

/**
 * Output the NetLabel domain mappings
 * @param mapping the domain mappings
 * @param count the number of domain mappings
 *
 * Helper function to be called by map_list().
 *
 */
static void map_list_print(struct nlbl_dommap *mapping, size_t count)
{
	uint32_t iter_a;
	struct nlbl_dommap_addr *iter_b;

	for (iter_a = 0; iter_a < count; iter_a++) {
		/* domain string */
		printf("domain:");
		if (mapping[iter_a].domain != NULL)
			printf("\"%s\",", mapping[iter_a].domain);
		else
			printf("DEFAULT,");
		/* protocol */
		switch (mapping[iter_a].proto_type) {
		case NETLBL_NLTYPE_UNLABELED:
			printf("UNLABELED");
			break;
		case NETLBL_NLTYPE_CIPSOV4:
			printf("CIPSOv4,%u", mapping[iter_a].proto.cv4_doi);
			break;
		case NETLBL_NLTYPE_ADDRSELECT:
			iter_b = mapping[iter_a].proto.addrsel;
			while (iter_b) {
				printf("address:");
				nlctl_addr_print(&iter_b->addr);
				printf(",protocol:");
				switch (iter_b->proto_type) {
				case NETLBL_NLTYPE_UNLABELED:
					printf("UNLABELED");
					break;
				case NETLBL_NLTYPE_CIPSOV4:
					printf("CIPSOv4,%u",
					       iter_b->proto.cv4_doi);
					break;
				default:
					printf("UNKNOWN(%u)",
					       iter_b->proto_type);
					break;
				}
				iter_b = iter_b->next;
			}
			break;
		default:
			printf("UNKNOWN(%u)", mapping[iter_a].proto_type);
			break;
		}
		if (iter_a + 1 < count)
			printf(" ");
	}
	printf("\n");
}

/**
 * Output the NetLabel domain mappings in human readable format
 * @param mapping the domain mappings
 * @param count the number of domain mappings
 *
 * Helper function to be called by map_list().
 *
 */
static void map_list_print_pretty(struct nlbl_dommap *mapping, size_t count)
{
	uint32_t iter_a;
	struct nlbl_dommap_addr *iter_b;

	printf("Configured NetLabel domain mappings (%zu)\n", count);
	for (iter_a = 0; iter_a < count; iter_a++) {
		/* domain string */
		printf(" domain: ");
		if (mapping[iter_a].domain != NULL)
			printf("\"%s\"\n", mapping[iter_a].domain);
		else
			printf("DEFAULT\n");
		/* protocol */
		switch (mapping[iter_a].proto_type) {
		case NETLBL_NLTYPE_UNLABELED:
			printf("   protocol: UNLABELED\n");
			break;
		case NETLBL_NLTYPE_CIPSOV4:
			printf("   protocol: CIPSOv4, DOI = %u\n",
			       mapping[iter_a].proto.cv4_doi);
			break;
		case NETLBL_NLTYPE_ADDRSELECT:
			iter_b = mapping[iter_a].proto.addrsel;
			while (iter_b) {
				printf("   address: ");
				nlctl_addr_print(&iter_b->addr);
				printf("\n"
				       "    protocol: ");
				switch (iter_b->proto_type) {
				case NETLBL_NLTYPE_UNLABELED:
					printf("UNLABELED\n");
					break;
				case NETLBL_NLTYPE_CIPSOV4:
					printf("CIPSOv4, DOI = %u\n",
					       iter_b->proto.cv4_doi);
					break;
				default:
					printf("UNKNOWN(%u)\n",
					       iter_b->proto_type);
					break;
				}
				iter_b = iter_b->next;
			}
			break;
		default:
			printf("UNKNOWN(%u)\n", mapping[iter_a].proto_type);
			break;
		}
	}
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
	struct nlbl_dommap *mapping;
	size_t count;
	uint32_t iter;

	/* get the list of mappings */
	ret_val = nlbl_mgmt_listall(NULL, &mapping);
	if (ret_val < 0)
		return ret_val;
	count = ret_val;

	/* get the default mapping */
	mapping = realloc(mapping, sizeof(struct nlbl_dommap) * (count + 1));
	if (mapping == NULL)
		goto list_return;
	memset(&mapping[count], 0, sizeof(struct nlbl_dommap));
	ret_val = nlbl_mgmt_listdef(NULL, &mapping[count]);
	if (ret_val < 0 && ret_val != -ENOENT)
		goto list_return;
	else if (ret_val == 0)
		count += 1;
	else
		ret_val = 0;

	/* display the results */
	if (opt_pretty != 0)
		map_list_print_pretty(mapping, count);
	else
		map_list_print(mapping, count);

list_return:
	if (mapping != NULL) {
		for (iter = 0; iter < count; iter++)
			if (mapping[iter].domain != NULL)
				free(mapping[iter].domain);
		free(mapping);
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
