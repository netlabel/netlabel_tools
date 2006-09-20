/*
 * Unlabeled Functions
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
#include <errno.h>

#include <libnetlabel.h>

#include "netlabelctl.h"

/**
 * unlbl_accept - Set the NetLabel accept flag
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Set the kernel's unlabeled packet allow flag.  Returns zero on success,
 * negative values on failure.
 *
 */
int unlbl_accept(int argc, char *argv[])
{
  int ret_val;
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

  ret_val = nlbl_unlbl_accept(NULL, flag);
  if (ret_val < 0)
    return ret_val;

  printf(MSG("Allow unlabeled packets : %s\n"), (flag?"on":"off"));

  return 0;
}

/**
 * unlbl_list - Query the NetLabel accept flag
 *
 * Description:
 * Query the unlabeled packet allow flag.  Returns zero on success, negative
 * values on failure.
 *
 */
int unlbl_list(void)
{
  int ret_val;
  uint8_t flag;

  ret_val = nlbl_unlbl_list(NULL, &flag);
  if (ret_val < 0)
    return ret_val;

  printf(MSG("Allow unlabeled packets : "));
  printf("%s\n", (flag?"on":"off"));

  return 0;
}

/**
 * unlbl_main - Entry point for the NetLabel unlabeled functions
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Parses the argument list and performs the requested operation.  Returns zero
 * on success, negative values on failure.
 *
 */
int unlbl_main(int argc, char *argv[])
{
  int ret_val;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;

  /* handle the request */
  if (strcmp(argv[0], "accept") == 0) {
    /* accept flag */
    ret_val = unlbl_accept(argc - 1, argv + 1);
  } else if (strcmp(argv[0], "list") == 0) {
    /* list */
    ret_val = unlbl_list();
  } else {
    /* unknown request */
    fprintf(stderr, MSG_ERR_MOD("unlbl", "unknown command\n"));
    ret_val = -EINVAL;
  }

  return ret_val;
}
