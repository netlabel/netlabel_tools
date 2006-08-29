/*
 * Domain/Protocol Mapping Functions
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
#include <linux/netlabel.h>

#include <libnetlabel.h>

#include "netlabelctl.h"

/**
 * map_add - Add a domain mapping to NetLabel
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Add the specified domain mapping to the NetLabel system.  Returns zero on
 * success, negative values on failure.
 *
 */
int map_add(int argc, char *argv[])
{
  int ret_val;
  unsigned int iter;
  unsigned int def_flag = 0;
  char *domain_str = NULL;
  nlbl_type domain_proto = NETLBL_NLTYPE_NONE;
  char *domain_proto_extra = NULL;
  mgmt_domain domain;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;

  /* parse the arguments */
  iter = 0;
  while (iter < argc && argv[iter] != NULL) {
    if (strncmp(argv[iter], "domain:", 7) == 0) {
      /* domain string */
      domain_str = argv[iter] + 7;
    } else if (strncmp(argv[iter], "protocol:", 9) == 0) {
      /* protocol specifics */
      if (strncmp(argv[iter] + 9, "cipsov4", 7) == 0)
        domain_proto = NETLBL_NLTYPE_CIPSOV4;
      else if (strncmp(argv[iter] + 9, "unlbl", 5) == 0)
        domain_proto = NETLBL_NLTYPE_UNLABELED;
      else
        domain_proto = NETLBL_NLTYPE_NONE;
      domain_proto_extra = strstr(argv[iter] + 9, ",");
      if (domain_proto_extra)
        domain_proto_extra++;
    } else if (strncmp(argv[iter], "default", 7) == 0) {
      /* default flag */
      def_flag = 1;
    } else
      return -EINVAL;
    iter++;
  }
  if ((domain_str == NULL && def_flag == 0) || 
      domain_proto == NETLBL_NLTYPE_NONE)
    return -EINVAL;

  /* generate the domain paramter */
  if (def_flag == 0) {
    domain.domain = domain_str;
    domain.domain_len = strlen(domain_str) + 1;
  } else {
    domain.domain = NULL;
    domain.domain_len = 0;
  }
  domain.proto_type = domain_proto;
  switch (domain_proto) {
  case NETLBL_NLTYPE_CIPSOV4:
    domain.proto.cipsov4.doi = (cv4_doi)atoi(domain_proto_extra);
    break;
  case NETLBL_NLTYPE_UNLABELED:
    break;
  default:
    return -EINVAL;
  }

  /* push the mapping into the kernel */
  ret_val = nlbl_mgmt_add(0, &domain, 1, def_flag);
  if (opt_pretty) {
    if (ret_val < 0)
      printf("Failed adding the domain mapping\n");
    else
      printf("Added the domain mapping\n");
  }

  return ret_val;
}

/**
 * map_del - Delete a domain mapping from NetLabel
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Remove the specified domain mapping from the NetLabel system.  Returns zero
 * on success, negative values on failure.
 *
 */
int map_del(int argc, char *argv[])
{
  int ret_val;
  unsigned int iter;
  unsigned int def_flag = 0;
  char *domain = NULL;
  size_t domain_len;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;

  /* parse the arguments */
  iter = 0;
  while (iter < argc && argv[iter] != NULL) {
    if (strncmp(argv[iter], "domain:", 7) == 0) {
      /* domain string */
      domain = argv[iter] + 7;
      domain_len = strlen(domain) + 1;
    } else if (strncmp(argv[iter], "default", 7) == 0) {
      /* default flag */
      def_flag = 1;
    } else
      return -EINVAL;
    iter++;
  }
  if (domain == NULL && def_flag == 0)
    return -EINVAL;

  /* remove the mapping from the kernel */
  ret_val = nlbl_mgmt_del(0, &domain, &domain_len, 1, def_flag);
  if (opt_pretty) {
    if (ret_val < 0)
      printf("Failed deleting the domain mapping\n");
    else
      printf("Deleted the domain mapping\n");
  }

  return ret_val;
}

/**
 * map_list - List the NetLabel domains mappings
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * List the configured NetLabel domain mappings.  Returns zero on success,
 * negative values on failure.
 *
 */
int map_list(int argc, char *argv[])
{
  int ret_val;
  mgmt_domain *domains = NULL;
  size_t count;
  unsigned int iter;

  /* get the mappings from the kernel */
  ret_val = nlbl_mgmt_list(0, &domains, &count, 0);
  if (ret_val < 0)
    goto list_return;

  /* display the results */
  if (opt_pretty) {
    printf("Configured NetLabel domain mappings (%u, not including DEFAULT)\n", count);
    for (iter = 0; iter < count; iter++) {
      /* domain string */
      printf(" domain: \"%s\"\n", domains[iter].domain);
      /* protocol */
      printf("   protocol: ");
      switch (domains[iter].proto_type) {
      case NETLBL_NLTYPE_UNLABELED:
        printf("UNLABELED\n");
        break;
      case NETLBL_NLTYPE_CIPSOV4:
        printf("CIPSOv4, DOI = %u\n", domains[iter].proto.cipsov4.doi);
        break;
      default:
        printf("UNKNOWN(%u)\n", domains[iter].proto_type);
        break;
      }
    }
  } else {
    for (iter = 0; iter < count; iter++) {
      /* domain string */
      printf("domain:\"%s\",", domains[iter].domain);
      /* protocol */
      switch (domains[iter].proto_type) {
      case NETLBL_NLTYPE_UNLABELED:
        printf("UNLABELED");
        break;
      case NETLBL_NLTYPE_CIPSOV4:
        printf("CIPSOv4,%u", domains[iter].proto.cipsov4.doi);
        break;
      default:
        printf("UNKNOWN(%u)", domains[iter].proto_type);
        break;
      }
      printf(" ");
    }
  }

  /* release the mapping memory */
  for (iter = 0; iter < count; iter++)
    if (domains[iter].domain)
      free(domains[iter].domain);
  free(domains);
  domains = NULL;

  /* get the default mapping from the kernel */
  ret_val = nlbl_mgmt_list(0, &domains, &count, 1);
  if (ret_val < 0)
    goto list_return;

  if (count > 0) {
    /* display the results */
    if (opt_pretty) {
      /* domain string */
      printf(" domain: DEFAULT\n");
      /* protocol */
      printf("   protocol: ");
      switch (domains[0].proto_type) {
      case NETLBL_NLTYPE_UNLABELED:
	printf("UNLABELED\n");
	break;
      case NETLBL_NLTYPE_CIPSOV4:
	printf("CIPSOv4, DOI = %u\n", domains[0].proto.cipsov4.doi);
	break;
      default:
	printf("UNKNOWN(%u)\n", domains[0].proto_type);
	break;
      }
    } else {
      /* domain string */
      printf("domain:DEFAULT,");
      /* protocol */
      switch (domains[0].proto_type) {
      case NETLBL_NLTYPE_UNLABELED:
	printf("UNLABELED");
	break;
      case NETLBL_NLTYPE_CIPSOV4:
	printf("CIPSOv4,%u", domains[0].proto.cipsov4.doi);
	break;
      default:
	printf("UNKNOWN(%u)", domains[0].proto_type);
	break;
      }
      printf("\n");
    }
  }

 list_return:
  if (domains) {
    for (iter = 0; iter < count; iter++)
      if (domains[iter].domain)
        free(domains[iter].domain);
    free(domains);
  }

  return ret_val;
}

/**
 * map_main - Entry point for the NetLabel mapping functions
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
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
    fprintf(stderr, "error[map]: unknown command\n");
    ret_val = -EINVAL;
  }

  return ret_val;
}
