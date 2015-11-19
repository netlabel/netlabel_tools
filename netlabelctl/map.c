/*
 * Domain/Protocol Mapping Functions
 *
 * Author: Paul Moore <paul@paul-moore.com>
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
			if (strncmp(argv[iter] + 9, "cipsov4", 7) == 0) {
				domain.proto_type = NETLBL_NLTYPE_CIPSOV4;
				domain.family = AF_INET;
			} else if (strncmp(argv[iter] + 9, "calipso", 7) == 0) {
				domain.proto_type = NETLBL_NLTYPE_CALIPSO;
				domain.family = AF_INET6;
			} else if (strncmp(argv[iter] + 9, "unlbl", 5) == 0) {
				domain.proto_type = NETLBL_NLTYPE_UNLABELED;
				domain.family = AF_UNSPEC;
			} else {
				return -EINVAL;
			}
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
	case NETLBL_NLTYPE_CALIPSO:
		if (domain_proto_extra == NULL)
			return -EINVAL;
		domain.proto.cal_doi = atoi(domain_proto_extra);
		break;
	case NETLBL_NLTYPE_UNLABELED:
		if (domain_proto_extra != NULL) {
			int family = atoi(domain_proto_extra);
			switch (family) {
			case 4:
				domain.family = AF_INET;
				break;
			case 6:
				domain.family = AF_INET6;
				break;
			default:
				return -EINVAL;
			}
		}
		break;
	}

	if (domain.family == AF_UNSPEC && addr.type != 0)
		domain.family = addr.type;

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
			if (mapping[iter_a].family == AF_INET)
				printf(",4");
			else if (mapping[iter_a].family == AF_INET6)
				printf(",6");
			break;
		case NETLBL_NLTYPE_CIPSOV4:
			printf("CIPSOv4,%u", mapping[iter_a].proto.cv4_doi);
			break;
		case NETLBL_NLTYPE_CALIPSO:
			printf("CALIPSO,%u", mapping[iter_a].proto.cal_doi);
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
				case NETLBL_NLTYPE_CALIPSO:
					printf("CALIPSO,%u",
					       iter_b->proto.cal_doi);
					break;
				default:
					printf("UNKNOWN(%u)",
					       iter_b->proto_type);
					break;
				}
				iter_b = iter_b->next;
				if (iter_b)
					printf(",");
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
			printf("\"%s\"", mapping[iter_a].domain);
		else
			printf("DEFAULT");
		/* family */
		if (mapping[iter_a].family == AF_INET)
			printf(" (IPv4)\n");
		else if (mapping[iter_a].family == AF_INET6)
			printf(" (IPv6)\n");
		else if (mapping[iter_a].family == AF_UNSPEC)
			printf(" (IPv4/IPv6)\n");
		/* protocol */
		switch (mapping[iter_a].proto_type) {
		case NETLBL_NLTYPE_UNLABELED:
			printf("   protocol: UNLABELED\n");
			break;
		case NETLBL_NLTYPE_CIPSOV4:
			printf("   protocol: CIPSOv4, DOI = %u\n",
			       mapping[iter_a].proto.cv4_doi);
			break;
		case NETLBL_NLTYPE_CALIPSO:
			printf("   protocol: CALIPSO, DOI = %u\n",
			       mapping[iter_a].proto.cal_doi);
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
				case NETLBL_NLTYPE_CALIPSO:
					printf("CALIPSO, DOI = %u\n",
					       iter_b->proto.cal_doi);
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
	int rc;
	struct nlbl_dommap *mapping, *mapping_new;
	size_t count, def_count;
	uint32_t iter;
	uint16_t *family, families[] = {AF_INET, AF_INET6, AF_UNSPEC /* terminator */};

	/* get the list of mappings */
	rc = nlbl_mgmt_listall(NULL, &mapping);
	if (rc < 0)
		return rc;
	count = rc;

	/* get the default mapping */
	mapping_new = realloc(mapping, sizeof(*mapping) * (count + 2));
	if (mapping_new == NULL)
		goto list_return;
	mapping = mapping_new;
	memset(&mapping[count], 0, sizeof(*mapping) * 2);

	for (family = families, def_count = 0; *family != AF_UNSPEC; family++) {
		rc = nlbl_mgmt_listdef(NULL, *family, &mapping[count + def_count]);
		if (rc < 0 && rc != -ENOENT)
			goto list_return;
		else if (rc == 0)
			def_count += 1;
		else
			rc = 0;
	}

	/* If both defaults are unlabeled then combine them into a single entry */
	if (def_count == 2 && mapping[count].proto_type == NETLBL_NLTYPE_UNLABELED &&
	    mapping[count + 1].proto_type == NETLBL_NLTYPE_UNLABELED) {
		mapping[count].family = AF_UNSPEC;
		def_count--;
	}
	count += def_count;

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
	return rc;
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
	int rc;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* handle the request */
	if (strcmp(argv[0], "add") == 0) {
		/* add a domain mapping */
		rc = map_add(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "del") == 0) {
		/* delete a domain mapping */
		rc = map_del(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "list") == 0) {
		/* list the domain mappings */
		rc = map_list(argc - 1, argv + 1);
	} else {
		/* unknown request */
		rc = -EINVAL;
	}

	return rc;
}
