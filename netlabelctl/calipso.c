/*
 * CALIPSO Functions
 *
 * Author: Paul Moore <paul@paul-moore.com>
 * Author: Huw Davies <huw@codeweavers.com>
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
 * Add a CALIPSO label mapping
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Add a CALIPSO label mapping to the NetLabel system.  Returns zero on
 * success, negative values on failure.
 *
 */
int calipso_add(int argc, char *argv[])
{
	int rc;
	uint32_t iter;
	uint32_t calipso_type = CALIPSO_MAP_UNKNOWN;
	nlbl_cal_doi doi = 0;
	struct nlbl_cal_lvl_a lvls = { .array = NULL, .size = 0 };
	struct nlbl_cal_cat_a cats = { .array = NULL, .size = 0 };
	char *token_ptr;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* parse the arguments */
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strcmp(argv[iter], "trans") == 0) {
			calipso_type = CALIPSO_MAP_TRANS;
		} else if (strcmp(argv[iter], "pass") == 0) {
			calipso_type = CALIPSO_MAP_PASS;
		} else if (strncmp(argv[iter], "doi:", 4) == 0) {
			/* doi */
			doi = atoi(argv[iter] + 4);
		} else if (strncmp(argv[iter], "levels:", 7) == 0) {
			/* levels */
			token_ptr = strtok(argv[iter] + 7, "=");
			while (token_ptr != NULL) {
				lvls.array = realloc(lvls.array,
						     sizeof(nlbl_cal_lvl) * 2 *
						     (lvls.size + 1));
				if (lvls.array == NULL) {
					rc = -ENOMEM;
					goto add_return;
				}
				/* XXX - should be more robust for bad input */
				lvls.array[lvls.size * 2] = atoi(token_ptr);
				token_ptr = strtok(NULL, ",");
				lvls.array[lvls.size * 2 + 1] = atoi(token_ptr);
				token_ptr = strtok(NULL, "=");
				lvls.size++;
			}
		} else if (strncmp(argv[iter], "categories:", 11) == 0) {
			/* categories */
			token_ptr = strtok(argv[iter] + 11, "=");
			while (token_ptr != NULL) {
				cats.array = realloc(cats.array,
						     sizeof(nlbl_cal_cat) * 2 *
						     (cats.size + 1));
				if (cats.array == NULL) {
					rc = -ENOMEM;
					goto add_return;
				}
				/* XXX - should be more robust for bad input */
				cats.array[cats.size * 2] = atoi(token_ptr);
				token_ptr = strtok(NULL, ",");
				cats.array[cats.size * 2 + 1] = atoi(token_ptr);
				token_ptr = strtok(NULL, "=");
				cats.size++;
			}
		} else
			return -EINVAL;
	}

	/* add the calipso mapping */
	switch (calipso_type) {
	case CALIPSO_MAP_TRANS:
		/* translated mapping */
		rc = nlbl_calipso_add_trans(NULL,
					    doi, &lvls, &cats);
		break;
	case CALIPSO_MAP_PASS:
		/* pass through mapping */
		rc = nlbl_calipso_add_pass(NULL, doi);
		break;
	default:
		rc = -EINVAL;
	}

add_return:
	if (lvls.array != NULL)
		free(lvls.array);
	if (cats.array != NULL)
		free(cats.array);
	return rc;
}

/**
 * Remove a CALIPSO label mapping
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Remove a CALIPSO label mapping from the NetLabel system.  Returns zero on
 * success, negative values on failure.
 *
 */
int calipso_del(int argc, char *argv[])
{
	uint32_t iter;
	nlbl_cal_doi doi = 0;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* parse the arguments */
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strncmp(argv[iter], "doi:", 4) == 0) {
			/* doi */
			doi = atoi(argv[iter] + 4);
		} else
			return -EINVAL;
	}

	/* delete the mapping */
	return nlbl_calipso_del(NULL, doi);
}

/**
 * List all of the CALIPSO label mappings
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * List the configured CALIPSO label mappings.  Returns zero on success,
 * negative values on failure.
 *
 */
static int calipso_list_all(void)
{
	int rc;
	uint32_t iter;
	nlbl_cal_doi *doi_list = NULL;
	nlbl_cal_mtype *mtype_list = NULL;
	size_t count;

	rc = nlbl_calipso_listall(NULL, &doi_list, &mtype_list);
	if (rc < 0)
		goto list_all_return;
	count = rc;

	if (opt_pretty != 0) {
		printf("Configured CALIPSO mappings (%zu)\n", count);
		for (iter = 0; iter < count; iter++) {
			/* doi value */
			printf(" DOI value : %u\n", doi_list[iter]);
			/* map type */
			printf("   mapping type : ");
			switch (mtype_list[iter]) {
			case CALIPSO_MAP_TRANS:
				printf("TRANSLATED\n");
				break;
			case CALIPSO_MAP_PASS:
				printf("PASS_THROUGH\n");
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
			case CALIPSO_MAP_TRANS:
				printf("TRANSLATED");
				break;
			case CALIPSO_MAP_PASS:
				printf("PASS_THROUGH");
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

	rc = 0;

list_all_return:
	if (doi_list != NULL)
		free(doi_list);
	if (mtype_list != NULL)
		free(mtype_list);
	return rc;
}

/**
 * List a specific CALIPSO DOI label mapping
 * @param doi the DOI value
 *
 * List the configured CALIPSO label mapping.  Returns zero on success,
 * negative values on failure.
 *
 */
static int calipso_list_doi(uint32_t doi)
{
	int rc;
	uint32_t iter;
	nlbl_cal_mtype maptype;
	struct nlbl_cal_lvl_a lvls = { .array = NULL, .size = 0 };
	struct nlbl_cal_cat_a cats = { .array = NULL, .size = 0 };

	rc = nlbl_calipso_list(NULL, doi, &maptype, &lvls, &cats);
	if (rc < 0)
		return rc;

	if (opt_pretty != 0) {
		printf("Configured CALIPSO mapping (DOI = %u)\n", doi);
		switch (maptype) {
		case CALIPSO_MAP_TRANS:
			printf(" type: TRANSLATED\n");
			/* levels */
			printf(" levels (%zu): \n", lvls.size);
			for (iter = 0; iter < lvls.size; iter++)
				printf("   %u = %u\n",
				       lvls.array[iter * 2],
				       lvls.array[iter * 2 + 1]);
			/* categories */
			printf(" categories (%zu): \n", cats.size);
			for (iter = 0; iter < cats.size; iter++)
				printf("   %u = %u\n",
				       cats.array[iter * 2],
				       cats.array[iter * 2 + 1]);
			break;
		case CALIPSO_MAP_PASS:
			printf(" type: PASS_THROUGH\n");
			break;
		}
	} else {
		switch (maptype) {
		case CALIPSO_MAP_TRANS:
			printf(" type:TRANSLATED");
			/* levels */
			printf(" levels:");
			for (iter = 0; iter < lvls.size; iter++) {
				printf("%u=%u",
				       lvls.array[iter * 2],
				       lvls.array[iter * 2 + 1]);
				if (iter + 1 < lvls.size)
					printf(",");
			}
			/* categories */
			printf(" categories:");
			for (iter = 0; iter < cats.size; iter++) {
				printf("%u=%u",
				       cats.array[iter * 2],
				       cats.array[iter * 2 + 1]);
				if (iter + 1 < cats.size)
					printf(",");
			}
			break;
		case CALIPSO_MAP_PASS:
			printf(" type:PASS_THROUGH");
			break;
		}
		printf("\n");
	}

	return 0;
}

/**
 * List the CALIPSO label mappings
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * List the configured CALIPSO label mappings.  Returns zero on success,
 * negative values on failure.
 *
 */
int calipso_list(int argc, char *argv[])
{
	uint32_t iter;
	uint32_t doi_flag = 0;
	nlbl_cal_doi doi = 0;

	/* parse the arguments */
	for (iter = 0; iter < argc && argv[iter] != NULL; iter++) {
		if (strncmp(argv[iter], "doi:", 4) == 0) {
			/* doi */
			doi = atoi(argv[iter] + 4);
			doi_flag = 1;
		} else
			return -EINVAL;
	}

	if (doi_flag != 0)
		return calipso_list_doi(doi);
	else
		return calipso_list_all();
}

/**
 * Entry point for the NetLabel CALIPSO functions
 * @param argc the number of arguments
 * @param argv the argument list
 *
 * Parses the argument list and performs the requested operation.  Returns zero
 * on success, negative values on failure.
 *
 */
int calipso_main(int argc, char *argv[])
{
	int rc;

	/* sanity checks */
	if (argc <= 0 || argv == NULL || argv[0] == NULL)
		return -EINVAL;

	/* handle the request */
	if (strcmp(argv[0], "add") == 0) {
		/* add */
		rc = calipso_add(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "del") == 0) {
		/* delete */
		rc = calipso_del(argc - 1, argv + 1);
	} else if (strcmp(argv[0], "list") == 0) {
		/* list */
		rc = calipso_list(argc - 1, argv + 1);
	} else {
		/* unknown request */
		rc = -EINVAL;
	}

	return rc;
}
