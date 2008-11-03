/*
 * NetLabel Library init/exit Functions
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
 * nlbl_init - Handle any NetLabel setup needed
 *
 * Description:
 * Initialize the NetLabel communication link, but do not open any general use
 * NetLabel handles.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_init(void)
{
	int ret_val;

	ret_val = nlbl_mgmt_init();
	if (ret_val < 0)
		return ret_val;

	ret_val = nlbl_cipsov4_init();
	if (ret_val < 0)
		return ret_val;

	ret_val = nlbl_unlbl_init();
	if (ret_val < 0)
		return ret_val;

	return 0;
}

/**
 * nlbl_exit - Handle any NetLabel cleanup
 *
 * Description:
 * Perform any cleanup duties for the NetLabel communication link, does not
 * close any handles.
 *
 */
void nlbl_exit(void)
{
	return;
}
