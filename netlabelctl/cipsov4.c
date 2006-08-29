/*
 * CIPSO/IPv4 Functions
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
 * cipsov4_add - Add a CIPSOv4 label mapping
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Add a CIPSOv4 label mapping to the NetLabel system.  Returns zero on
 * success, negative values on failure.
 *
 */
int cipsov4_add(int argc, char *argv[])
{
  int ret_val;
  unsigned int arg_iter;
  unsigned int cipso_type = 0;
  cv4_doi doi = 0;
  cv4_tag_array tags = { NULL, 0 };
  cv4_lvl_array lvls = { NULL, 0 };
  cv4_cat_array cats = { NULL, 0 };
  char *token_ptr;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;
  
  /* parse the arguments */
  arg_iter = 0;
  while (arg_iter < argc && argv[arg_iter] != NULL) {
    if (strcmp(argv[arg_iter], "std") == 0) {
      cipso_type = 1;
    } else if (strcmp(argv[arg_iter], "pass") == 0) {
      cipso_type = 2;
    } else if (strncmp(argv[arg_iter], "doi:", 4) == 0) {
      /* doi */
      doi = (cv4_doi)atoi(argv[arg_iter] + 4);
    } else if (strncmp(argv[arg_iter], "tags:", 5) == 0) {
      /* tags */
      token_ptr = strtok(argv[arg_iter] + 5, ",");
      while (token_ptr != NULL) {
        tags.array = realloc(tags.array, sizeof(cv4_tag) * (tags.size + 1));
        if (tags.array == NULL) {
          ret_val = -ENOMEM;
          goto add_return;
        }
        tags.array[tags.size++] = (cv4_tag)atoi(token_ptr);
        token_ptr = strtok(NULL, ",");
      }
    } else if (strncmp(argv[arg_iter], "levels:", 7) == 0) {
      /* levels */
      token_ptr = strtok(argv[arg_iter] + 7, "=");
      while (token_ptr != NULL) {
        lvls.array = realloc(lvls.array,
			     sizeof(cv4_lvl) * 2 * (lvls.size + 1));
        if (lvls.array == NULL) {
          ret_val = -ENOMEM;
          goto add_return;
        }
        lvls.array[lvls.size * 2] = (cv4_lvl)atoi(token_ptr);
        token_ptr = strtok(NULL, ",");
        lvls.array[lvls.size * 2 + 1] = (cv4_lvl)atoi(token_ptr);
        token_ptr = strtok(NULL, "=");
        lvls.size++;
      }
    } else if (strncmp(argv[arg_iter], "categories:", 11) == 0) {
      /* categories */
      token_ptr = strtok(argv[arg_iter] + 11, "=");
      while (token_ptr != NULL) {
        cats.array = realloc(cats.array,
			     sizeof(cv4_cat) * 2 * (cats.size + 1));
        if (cats.array == NULL) {
          ret_val = -ENOMEM;
          goto add_return;
        }
        cats.array[cats.size * 2] = (cv4_cat)atoi(token_ptr);
        token_ptr = strtok(NULL, ",");
        cats.array[cats.size * 2 + 1] = (cv4_cat)atoi(token_ptr);
        token_ptr = strtok(NULL, "=");
        cats.size++;
      }
    } else
      return -EINVAL;
    arg_iter++;
  }

  /* push the mapping into the kernel */
  switch (cipso_type) {
  case 1:
    /* standard mapping */
    ret_val = nlbl_cipsov4_add_std(0, doi, &tags, &lvls, &cats);
    break;
  case 2:
    /* pass through mapping */
    ret_val = nlbl_cipsov4_add_pass(0, doi, &tags);
    break;
  default:
    ret_val = -EINVAL;
  }
  if (opt_pretty) {
    if (ret_val < 0)
      printf("Failed adding the CIPSOv4 mapping\n");
    else
      printf("Added the CIPSOv4 mapping\n");
  }

  /* cleanup and return */
 add_return:
  if (tags.array)
    free(tags.array);
  if (lvls.array)
    free(lvls.array);
  if (cats.array)
    free(cats.array);

  return ret_val;
}

/**
 * cipsov4_del - Remove a CIPSOv4 label mapping
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Remove a CIPSOv4 label mapping from the NetLabel system.  Returns zero on
 * success, negative values on failure.
 *
 */
int cipsov4_del(int argc, char *argv[])
{
  int ret_val;
  unsigned int arg_iter;
  cv4_doi doi = 0;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;
  
  /* parse the arguments */
  arg_iter = 0;
  while (arg_iter < argc && argv[arg_iter] != NULL) {
    if (strncmp(argv[arg_iter], "doi:", 4) == 0) {
      /* doi */
      doi = (cv4_doi)atoi(argv[arg_iter] + 4);
    } else
      return -EINVAL;
    arg_iter++;
  }

  /* delete the mapping */
  ret_val = nlbl_cipsov4_del(0, doi);
  if (opt_pretty) {
    if (ret_val < 0)
      printf("Failed to remove the CIPSOv4 mapping\n");
    else
      printf("Removed the CIPSOv4 mapping\n");
  }

  return 0;
}

/**
 * cipsov4_list - List the CIPSOv4 label mappings
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * List the configured CIPSOv4 label mappings.  Returns zero on success,
 * negative values on failure.
 *
 */
int cipsov4_list(int argc, char *argv[])
{
  int ret_val;
  unsigned int iter;
  unsigned int doi_set = 0;
  cv4_doi doi = 0;
  cv4_doi *doi_list = NULL;
  cv4_maptype *mtype_list = NULL;
  cv4_maptype maptype;
  size_t count;
  cv4_tag_array tags = { NULL, 0 };
  cv4_lvl_array lvls = { NULL, 0 };
  cv4_cat_array cats = { NULL, 0 };

  /* parse the arguments */
  iter = 0;
  while (iter < argc && argv[iter] != NULL) {
    if (strncmp(argv[iter], "doi:", 4) == 0) {
      /* doi */
      doi = (cv4_doi)atoi(argv[iter] + 4);
      doi_set = 1;
    } else
      return -EINVAL;
    iter++;
  }

  /* fetch the information from the kernel and display the results */
  if (doi_set == 0) {
    /* list all the mappings */
    ret_val = nlbl_cipsov4_list_all(0, &doi_list, &mtype_list, &count);
    if (ret_val < 0)
      goto list_return;

    if (opt_pretty) {
      printf("Configured CIPSOv4 mappings (%u)\n", count);
      for (iter = 0; iter < count; iter++) {
        /* doi value */
        printf(" DOI value : %u\n", doi_list[iter]);
        /* map type */
        printf("   mapping type : ");
        switch (mtype_list[iter]) {
        case CIPSO_V4_MAP_STD:
          printf("STANDARD\n");
          break;
        case CIPSO_V4_MAP_PASS:
          printf("PASS THROUGH\n");
          break;
        default:
          printf("UNKNOWN(%u)\n", mtype_list[iter]);
          break;
        }
      }
    } else {
      for (iter = 0; iter < count; iter++) {
        /* doi value */
        printf("%u,", doi_list[iter]);
        /* map type */
        switch (mtype_list[iter]) {
        case CIPSO_V4_MAP_STD:
          printf("STANDARD");
          break;
        case CIPSO_V4_MAP_PASS:
          printf("PASS_THROUGH\n");
          break;
        default:
          printf("UNKNOWN(%u)", mtype_list[iter]);
          break;
        }
        if (iter + 1 < count)
          printf(" ");
      }
      printf("\n");
    }
  } else {
    /* list a specific mapping */
    ret_val = nlbl_cipsov4_list(0, doi, &maptype, &tags, &lvls, &cats);
    if (ret_val < 0)
      goto list_return;

    if (opt_pretty) {
      printf("Configured CIPSOv4 mapping (DOI = %u)\n", doi);
      switch (maptype) {
      case CIPSO_V4_MAP_STD:
	/* tags */
	printf(" tags (%u): \n", tags.size);
	for (iter = 0; iter < tags.size; iter++) {
	  switch (tags.array[iter]) {
	  case 1:
	    printf("   RESTRICTED BITMAP\n");
	    break;
	  case 2:
	    printf("   ENUMERATED\n");
	    break;
	  case 5:
	    printf("   RANGED\n");
	    break;
	  case 6:
	    printf("   PERMISSIVE BITMAP\n");
	    break;
	  case 7:
	    printf("   FREEFORM\n");
	    break;
	  default:
	    printf("   UNKNOWN(%u)\n", tags.array[iter]);
	    break;
	  }
	}
	/* levels */
	printf(" levels (%u): \n", lvls.size);
	for (iter = 0; iter < lvls.size; iter++)
	  printf("   %u = %u\n", 
		 lvls.array[iter * 2],
		 lvls.array[iter * 2 + 1]);
	/* categories */
	printf(" categories (%u): \n", cats.size);
	for (iter = 0; iter < cats.size; iter++)
	  printf("   %u = %u\n",
		 cats.array[iter * 2],
		 cats.array[iter * 2 + 1]);
	break;
      case CIPSO_V4_MAP_PASS:
	/* tags */
	printf(" tags (%u): \n", tags.size);
	for (iter = 0; iter < tags.size; iter++) {
	  switch (tags.array[iter]) {
	  case 1:
	    printf("   RESTRICTED BITMAP\n");
	    break;
	  case 2:
	    printf("   ENUMERATED\n");
	    break;
	  case 5:
	    printf("   RANGED\n");
	    break;
	  case 6:
	    printf("   PERMISSIVE BITMAP\n");
	    break;
	  case 7:
	    printf("   FREEFORM\n");
	    break;
	  default:
	    printf("   UNKNOWN(%u)\n", tags.array[iter]);
	    break;
	  }
	}
	break;
      default:
	printf(" unknown mapping type\n");
      }
    } else {
      switch (maptype) {
      case CIPSO_V4_MAP_STD:
	/* tags */
	printf("tags:");
	for (iter = 0; iter < tags.size; iter++) {
	  printf("%u", tags.array[iter]);
	  if (iter + 1 < tags.size)
	    printf(",");
	}
	/* levels */
	printf(" levels:");
	for (iter = 0; iter < lvls.size; iter++) {
	  printf("%u=%u", lvls.array[iter * 2], lvls.array[iter * 2 + 1]);
	  if (iter + 1 < lvls.size)
	    printf(",");
	}
	/* categories */
	printf(" categories:");
	for (iter = 0; iter < cats.size; iter++) {
	  printf("%u=%u", cats.array[iter * 2], cats.array[iter * 2 + 1]);
	  if (iter + 1 < cats.size)
	    printf(",");
	}
	break;
      case CIPSO_V4_MAP_PASS:
	/* tags */
	printf("tags:");
	for (iter = 0; iter < tags.size; iter++) {
	  printf("%u", tags.array[iter]);
	  if (iter + 1 < tags.size)
	    printf(",");
	}
	break;
      default:
	printf("unknown mapping type\n");
      }
      printf("\n");
    }
  }

  ret_val = 0;

 list_return:
  if (doi_list)
    free(doi_list);
  if (mtype_list)
    free(mtype_list);

  return ret_val;
}

/**
 * cipsov4_main - Entry point for the NetLabel CIPSO/IPv4 functions
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Parses the argument list and performs the requested operation.  Returns zero
 * on success, negative values on failure.
 *
 */
int cipsov4_main(int argc, char *argv[])
{
  int ret_val;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;

  /* handle the request */
  if (strcmp(argv[0], "add") == 0) {
    /* add */
    ret_val = cipsov4_add(argc - 1, argv + 1);
  } else if (strcmp(argv[0], "del") == 0) {
    /* delete */
    ret_val = cipsov4_del(argc - 1, argv + 1);
  } else if (strcmp(argv[0], "list") == 0) {
    /* list */
    ret_val = cipsov4_list(argc - 1, argv + 1);
  } else {
    /* unknown request */
    fprintf(stderr, "error[cipsov4]: unknown command\n");
    ret_val = -EINVAL;
  }

  return ret_val;
}
