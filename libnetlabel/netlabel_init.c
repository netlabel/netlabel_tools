/** @file
 * NetLabel Library init/exit Functions
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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/types.h>

#include <libnetlabel.h>

#include "netlabel_internal.h"
#include "mod_mgmt.h"
#include "mod_unlabeled.h"
#include "mod_cipsov4.h"

/**
 * Handle any NetLabel setup needed
 *
 * Initialize the NetLabel communication link, but do not open any general use
 * NetLabel handles.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_init(void)
{
	int rc;

	nlmsg_set_default_size(8192);

	rc = nlbl_mgmt_init();
	if (rc < 0)
		return rc;

	rc = nlbl_cipsov4_init();
	if (rc < 0)
		return rc;

	rc = nlbl_unlbl_init();
	if (rc < 0)
		return rc;

	return 0;
}

/**
 * Handle any NetLabel cleanup
 *
 * Perform any cleanup duties for the NetLabel communication link, does not
 * close any handles.
 *
 */
void nlbl_exit(void)
{
	return;
}
