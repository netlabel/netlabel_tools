/*
 * NetLabel Library init/exit Functions
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <asm/types.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/netlabel.h>

#include <libnetlabel.h>

#include "mod_mgmt.h"
#include "mod_cipsov4.h"
#include "mod_unlabeled.h"

/**
 * nlbl_netlink_init - Handle any NETLINK setup needed
 *
 * Description:
 * Initialize the NETLINK communication link, but do not open any general use
 * sockets.  Returns zero on success, negative values on failure.
 *
 */
int nlbl_netlink_init(void)
{
  int ret_val;

  ret_val = nlbl_mgmt_init();
  if (ret_val < 0)
    goto init_failure;

  ret_val = nlbl_cipsov4_init();
  if (ret_val < 0)
    goto init_failure;

  ret_val = nlbl_unlabeled_init();
  if (ret_val < 0)
    goto init_failure;

  return 0;

 init_failure:
  return ret_val;
}

/**
 * nlbl_netlink_exit - Handle any NETLINK cleanup
 *
 * Description:
 * Perform any cleanup duties for the NETLINK communication link.
 *
 */
void nlbl_netlink_exit(void)
{
  return;
}
