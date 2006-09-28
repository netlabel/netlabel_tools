/*
 * NetLabel Control Utility, netlabelctl
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
#include <unistd.h>
#include <getopt.h>

#include <libnetlabel.h>

#include "netlabelctl.h"

/* return values */
#define RET_OK        0
#define RET_ERR       1
#define RET_USAGE     2

/* option variables */
uint32_t opt_verbose = 0;
uint32_t opt_timeout = 10;
uint32_t opt_pretty = 0;

/* program name */
char *nlctl_name = NULL;

/**
 * nlctl_usage_print - Display usage information
 * @fp: the output file pointer
 *
 * Description:
 * Display brief usage information.
 * 
 */
static void nlctl_usage_print(FILE *fp)
{
  fprintf(fp, "usage: %s [<flags>] <module> [<commands>]\n", nlctl_name);
}

/**
 * nlctl_ver_print - Display version information
 * @fp: the output file pointer
 *
 * Description:
 * Display the version string.
 *
 */
static void nlctl_ver_print(FILE *fp)
{
  fprintf(fp,
	  "NetLabel Control Utility, version %s (%s)\n",
	  NETLBL_VER_STRING,
	  NETLBL_VER_DATE);
}

/**
 * nlctl_help_print - Display help information
 * @fp: the output file pointer
 *
 * Description:
 * Display help and usage information.
 * 
 */
static void nlctl_help_print(FILE *fp)
{
  nlctl_ver_print(fp);
  fprintf(fp,
          " Usage: %s [<flags>] <module> [<commands>]\n"
          "\n"
          " Flags:\n"
          "   -h        : help/usage message\n"
          "   -p        : make the output pretty\n"
          "   -t <secs> : timeout\n"
          "   -v        : verbose mode\n"
          "\n"
          " Modules and Commands:\n"
          "  mgmt : NetLabel management\n"
          "    version\n"
          "    protocols\n"
	  "  map : Domain/Protocol mapping\n"
          "    add default|domain:<domain> protocol:<protocol>[,<extra>]\n"
          "    del default|domain:<domain>\n"
          "    list\n"
          "  unlbl : Unlabeled packet handling\n"
          "    accept on|off\n"
	  "    list\n"
          "  cipsov4 : CIPSO/IPv4 packet handling\n"
          "    add std doi:<DOI> tags:<T1>,<Tn>\n"
	  "            levels:<LL1>=<RL1>,<LLn>=<RLn>\n"
          "            categories:<LC1>=<RC1>,<LCn>=<RCn>\n"
	  "    add pass doi:<DOI> tags:<T1>,<Tn>\n"
          "    del doi:<DOI>\n"
          "    list [doi:<DOI>]\n"
          "\n",
	  nlctl_name);
}

/**
 * main
 */
int main(int argc, char *argv[])
{
  int ret_val = RET_ERR;
  int arg_iter;
  main_function_t *module_main = NULL;
  char *module_name;

  /* sanity checks */
  if (argc < 2) {
    nlctl_usage_print(stderr);
    return RET_USAGE;
  }

  /* save of the invoked program name */
  nlctl_name = strrchr(argv[0], '/');
  if (nlctl_name == NULL)
    nlctl_name = argv[0];
  else if (nlctl_name[0] == '/')
    nlctl_name += 1;
  else
    nlctl_name = strdup("unknown");

  /* get the command line arguments */
  do {
    arg_iter = getopt(argc, argv, "hvt:pV");
    switch (arg_iter) {
    case 'h':
      /* help */
      nlctl_help_print(stdout);
      return RET_OK;
      break;
    case 'v':
      /* verbose */
      opt_verbose = 1;
      break;
    case 'p':
      /* pretty */
      opt_pretty = 1;
      break;
    case 't':
      /* timeout */
      if (atoi(optarg) < 0) {
        nlctl_usage_print(stderr);
        return RET_USAGE;
      }
      opt_timeout = atoi(optarg);
      break;
    case 'V':
      /* version */
      nlctl_ver_print(stdout);
      return RET_OK;
      break;
    }
  } while (arg_iter > 0);

  /* perform any setup we have to do */
  ret_val = nlbl_init();
  if (ret_val < 0) {
    fprintf(stderr, MSG_ERR("failed to initialize the NetLabel library\n"));
    goto exit;
  }
  nlbl_comm_timeout(opt_timeout);

  module_name = argv[optind];
  if (!module_name)
    goto exit;

  /* transfer control to the module */
  if (!strcmp(module_name, "mgmt")) {
    module_main = mgmt_main;
  } else if (!strcmp(module_name, "map")) {
    module_main = map_main;
  } else if (!strcmp(module_name, "unlbl")) {
    module_main = unlbl_main;
  } else if (!strcmp(module_name, "cipsov4")) {
    module_main = cipsov4_main;
  } else {
    fprintf(stderr, MSG_ERR("unknown or missing module '%s'\n"), module_name);
    goto exit;
  }
  ret_val = module_main(argc - optind - 1, argv + optind + 1);
  if (ret_val < 0) {
    fprintf(stderr, MSG_ERR("%s\n"), strerror(-ret_val));
    ret_val = RET_ERR;
  } else
    ret_val = RET_OK;

  nlbl_exit();

exit:
  return ret_val;
}
