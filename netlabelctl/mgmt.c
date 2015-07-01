/*
 * Management Functions
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libnetlabel.h>

#include "netlabelctl.h"

/**
 * Display a list of the kernel's NetLabel protocols
 *
 * Request the kernel's supported NetLabel protocols and display the list to
 * the user.  Returns zero on success, negative values on failure.
 *
 */
int mgmt_protocols(void)
{
	int rc;
	nlbl_proto *list = NULL;
	size_t count;
	uint32_t iter;

	rc = nlbl_mgmt_protocols(NULL, &list);
	if (rc < 0)
		return rc;
	count = rc;

	printf(MSG("NetLabel protocols : "));
	for (iter = 0; iter < count; iter++) {
		switch (list[iter]) {
		case NETLBL_NLTYPE_UNLABELED:
			printf("UNLABELED");
			break;
		case NETLBL_NLTYPE_RIPSO:
			printf("RIPSO");
			break;
		case NETLBL_NLTYPE_CIPSOV4:
			printf("CIPSOv4");
			break;
		case NETLBL_NLTYPE_CIPSOV6:
			printf("CIPSOv6");
			break;
		default:
			printf("UNKNOWN(%u)", list[iter]);
			break;
		}
		if (iter + 1 < count)
			printf("%s", (opt_pretty ? " " : ","));
	}
	printf("\n");

	if (list != NULL)
		free(list);
	return 0;
}

/**
 * Display the kernel's NetLabel version
 *
 * Request the kernel's NetLabel version string and display it to the user.
 * Returns zero on success, negative values on failure.
 *
 */
int mgmt_version(void)
{
	int rc;
	uint32_t kernel_ver;

	rc = nlbl_mgmt_version(NULL, &kernel_ver);
	if (rc < 0)
		return rc;

	if (opt_pretty != 0) {
		printf("Supported NetLabel protocol versions\n"
		       "  kernel : %u\n"
		       "  %s : %u\n",
		       kernel_ver, nlctl_name, NETLBL_PROTO_VERSION);
	} else
		printf("%u\n", kernel_ver);

	return 0;
}

/**
 * Entry point for the NetLabel management functions
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Description:
 * Parses the argument list and performs the requested operation.  Returns zero
 * on success, negative values on failure.
 *
 */
int mgmt_main(int argc, char *argv[])
{
	int rc;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* handle the request */
	if (strcmp(argv[0], "version") == 0) {
		/* kernel version */
		rc = mgmt_version();
	} else if (strcmp(argv[0], "protocols") == 0) {
		/* module list */
		rc = mgmt_protocols();
	} else {
		/* unknown request */
		rc = -EINVAL;
	}

	return rc;
}
