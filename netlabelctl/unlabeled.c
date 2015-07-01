/*
 * Unlabeled Functions
 *
 * Author: Paul Moore <paul@paul-moore.com>
 *
 */

/*
 * (c) Copyright Hewlett-Packard Development Company, L.P., 2006, 2007
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
 * Set the NetLabel accept flag
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Set the kernel's unlabeled packet allow flag.  Returns zero on success,
 * negative values on failure.
 *
 */
int unlbl_accept(int argc, char *argv[])
{
	int rc;
	uint8_t flag;

	/* sanity check */
	if (argc != 1 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* set or reset the flag? */
	if (strcasecmp(argv[0], "on") == 0 || strcmp(argv[0], "1") == 0)
		flag = 1;
	else if (strcasecmp(argv[0], "off") == 0 || strcmp(argv[0], "0") == 0)
		flag = 0;
	else
		return -EINVAL;

	rc = nlbl_unlbl_accept(NULL, flag);
	if (rc < 0)
		return rc;

	return 0;
}

/**
 * Query the NetLabel unlabeled module and display the results
 *
 * Query the unlabeled module and display the results.  Returns zero on
 * success, negative values on failure.
 *
 */
int unlbl_list(void)
{
	int rc;
	uint8_t flag;
	struct nlbl_addrmap *addr_p = NULL, *addr_p_new;
	struct nlbl_addrmap *addrdef_p = NULL;
	struct nlbl_addrmap *iter_p;
	size_t count;
	uint32_t iter;

	/* display the accept flag */
	rc = nlbl_unlbl_list(NULL, &flag);
	if (rc < 0)
		return rc;
	if (opt_pretty != 0)
		printf("Accept unlabeled packets : %s\n",
		       (flag ? "on" : "off"));
	else
		printf("accept:%s", (flag ? "on" : "off"));

	/* get the static label mappings */
	rc = nlbl_unlbl_staticlist(NULL, &addr_p);
	if (rc < 0)
		return rc;
	count = rc;
	rc = nlbl_unlbl_staticlistdef(NULL, &addrdef_p);
	if (rc > 0) {
		addr_p_new = realloc(addr_p, sizeof(*addr_p) * (count + rc));
		if (addr_p_new == NULL)
			goto list_return;
		addr_p = addr_p_new;
		memcpy(&addr_p[count], addrdef_p, sizeof(*addr_p) * rc);
		count += rc;
	}

	/* display the static label mappings */
	if (opt_pretty != 0) {
		printf("Configured NetLabel address mappings (%zu)\n", count);
		for (iter = 0; iter < count; iter++) {
			iter_p = &addr_p[iter];
			/* interface */
			if (iter == 0 ||
			    iter_p->dev == NULL ||
			    strcmp(addr_p[iter - 1].dev, iter_p->dev) != 0) {
				printf(" interface: ");
				if (iter_p->dev != NULL)
					printf("%s\n", iter_p->dev);
				else
					printf("DEFAULT\n");
			}
			/* address */
			printf("   address: ");
			nlctl_addr_print(&iter_p->addr);
			printf("\n");
			/* label */
			printf("    label: \"%s\"\n", iter_p->label);
		}
	} else {
		if (count > 0)
			printf(" ");
		for (iter = 0; iter < count; iter++) {
			iter_p = &addr_p[iter];
			/* interface */
			printf("interface:");
			if (iter_p->dev != NULL)
				printf("%s,", iter_p->dev);
			else
				printf("DEFAULT,");
			/* address */
			printf("address:");
			nlctl_addr_print(&iter_p->addr);
			printf(",");
			/* label */
			printf("label:\"%s\"", iter_p->label);
			if (iter + 1 < count)
				printf(" ");
		}
		printf("\n");
	}

list_return:
	if (addr_p != NULL) {
		for (iter = 0; iter < count; iter++) {
			if (addr_p[iter].dev != NULL)
				free(addr_p[iter].dev);
			if (addr_p[iter].label != NULL)
				free(addr_p[iter].label);
		}
		free(addr_p);
	}
	return rc;
}

/**
 * Add a static/fallback label configuration
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Add a fallback label configuration to the kernel.  Returns zero on success,
 * negative values on failure.
 *
 */
int unlbl_add(int argc, char *argv[])
{
	uint32_t iter;
	uint8_t def_flag = 0;
	nlbl_netdev dev = NULL;
	struct nlbl_netaddr addr;
	nlbl_secctx label = NULL;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	memset(&addr, 0, sizeof(addr));

	/* parse the arguments */
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strncmp(argv[iter], "interface:", 10) == 0) {
			dev = argv[iter] + 10;
		} else if (strncmp(argv[iter], "default", 7) == 0) {
			def_flag = 1;
		} else if (strncmp(argv[iter], "label:", 6) == 0) {
			label = argv[iter] + 6;
		} else if (strncmp(argv[iter], "address:", 8) == 0) {
			if (nlctl_addr_parse(argv[iter] + 8, &addr) != 0)
				return -EINVAL;
		}
	}

	/* add the mapping */
	if (def_flag != 0)
		return nlbl_unlbl_staticadddef(NULL, &addr, label);
	else
		return nlbl_unlbl_staticadd(NULL, dev, &addr, label);
}

/**
 * Delete a static/fallback label configuration
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Deletes a fallback label configuration to the kernel.  Returns zero on
 * success, negative values on failure.
 *
 */
int unlbl_del(int argc, char *argv[])
{
	uint32_t iter;
	uint8_t def_flag = 0;
	nlbl_netdev dev = NULL;
	struct nlbl_netaddr addr;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	memset(&addr, 0, sizeof(addr));

	/* parse the arguments */
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strncmp(argv[iter], "interface:", 10) == 0) {
			dev = argv[iter] + 10;
		} else if (strncmp(argv[iter], "default", 7) == 0) {
			def_flag = 1;
		} else if (strncmp(argv[iter], "address:", 8) == 0) {
			if (nlctl_addr_parse(argv[iter] + 8, &addr) != 0)
				return -EINVAL;
		}
	}

	/* add the mapping */
	if (def_flag != 0)
		return nlbl_unlbl_staticdeldef(NULL, &addr);
	else
		return nlbl_unlbl_staticdel(NULL, dev, &addr);
}

/**
 * Entry point for the NetLabel unlabeled functions
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Parses the argument list and performs the requested operation.  Returns zero
 * on success, negative values on failure.
 *
 */
int unlbl_main(int argc, char *argv[])
{
	int rc;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* handle the request */
	if (strcmp(argv[0], "accept") == 0) {
		/* accept flag */
		rc = unlbl_accept(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "list") == 0) {
		/* list */
		rc = unlbl_list();
	} else if (strcmp(argv[0], "add") == 0) {
		/* add */
		rc = unlbl_add(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "del") == 0) {
		/* del */
		rc = unlbl_del(argc - 1, argv + 1);
	} else {
		/* unknown request */
		rc = -EINVAL;
	}

	return rc;
}
