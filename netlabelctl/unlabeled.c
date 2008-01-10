/*
 * Unlabeled Functions
 *
 * Author: Paul Moore <paul.moore@hp.com>
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

  return 0;
}

/**
 * unlbl_list - Query the NetLabel unlabeled module and display the results
 *
 * Description:
 * Query the unlabeled module and display the results.  Returns zero on
 * success, negative values on failure.
 *
 */
int unlbl_list(void)
{
  int ret_val;
  uint8_t flag;
  nlbl_addrmap *addr_p = NULL;
  nlbl_addrmap *addrdef_p = NULL;
  nlbl_addrmap *iter_p;
  size_t count;
  uint32_t iter;
  char addr_s[80];
  socklen_t addr_s_len = 80;
  uint32_t mask_size;
  uint32_t mask_off;

  /* display the accept flag */
  ret_val = nlbl_unlbl_list(NULL, &flag);
  if (ret_val < 0)
    return ret_val;
  if (opt_pretty)
    printf("Accept unlabeled packets : %s\n", (flag ? "on" : "off"));
  else
    printf("accept:%s", (flag ? "on" : "off"));

  /* get the static label mappings */
  ret_val = nlbl_unlbl_staticlist(NULL, &addr_p);
  if (ret_val < 0)
    goto list_return;
  count = ret_val;

  /* get the default static label mapping */
  ret_val = nlbl_unlbl_staticlistdef(NULL, &addrdef_p);
  if (ret_val > 0) {
    addr_p = realloc(addr_p, sizeof(nlbl_addrmap) * (count + ret_val));
    if (addr_p == NULL)
      goto list_return;
    memcpy(&addr_p[count], addrdef_p, sizeof(nlbl_addrmap) * ret_val);
    count += ret_val;
  }

  /* display the static label mappings */
  if (opt_pretty) {
    printf("Configured NetLabel address mappings (%u)\n", count);
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
      switch (iter_p->addr.type) {
      case AF_INET:
	iter_p->addr.mask.v4.s_addr = ntohl(iter_p->addr.mask.v4.s_addr);
	for (mask_size = 0; iter_p->addr.mask.v4.s_addr != 0; mask_size++)
	  iter_p->addr.mask.v4.s_addr <<= 1;
	printf("%s/%u\n",
	       inet_ntop(AF_INET, &iter_p->addr.addr.v4, addr_s, addr_s_len),
	       mask_size);
        break;
      case AF_INET6:
	for (mask_size = 0, mask_off = 0; mask_off < 4; mask_off++) {
	  iter_p->addr.mask.v6.s6_addr32[mask_off] =
	    ntohl(iter_p->addr.mask.v6.s6_addr32[mask_off]);
	  while (iter_p->addr.mask.v6.s6_addr32[mask_off] != 0) {
	    mask_size++;
	    iter_p->addr.mask.v6.s6_addr32[mask_off] <<= 1;
	  }
	}
	printf("%s/%u\n",
	       inet_ntop(AF_INET6, &iter_p->addr.addr.v6, addr_s, addr_s_len),
	       mask_size);
        break;
      default:
        printf("UNKNOWN(%u)\n", iter_p->addr.type);
        break;
      }
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
      switch (iter_p->addr.type) {
      case AF_INET:
	iter_p->addr.mask.v4.s_addr = ntohl(iter_p->addr.mask.v4.s_addr);
	for (mask_size = 0; iter_p->addr.mask.v4.s_addr != 0; mask_size++)
	  iter_p->addr.mask.v4.s_addr <<= 1;
	printf("%s/%u,",
	       inet_ntop(AF_INET, &iter_p->addr.addr.v4, addr_s, addr_s_len),
	       mask_size);
        break;
      case AF_INET6:
	for (mask_size = 0, mask_off = 0; mask_off < 4; mask_off++) {
	  iter_p->addr.mask.v6.s6_addr32[mask_off] =
	    ntohl(iter_p->addr.mask.v6.s6_addr32[mask_off]);
	  while (iter_p->addr.mask.v6.s6_addr32[mask_off] != 0) {
	    mask_size++;
	    iter_p->addr.mask.v6.s6_addr32[mask_off] <<= 1;
	  }
	}
	printf("%s/%u,",
	       inet_ntop(AF_INET6, &iter_p->addr.addr.v6, addr_s, addr_s_len),
	       mask_size);
        break;
      default:
        printf("UNKNOWN(%u),", iter_p->addr.type);
        break;
      }
      /* label */
      printf("label:\"%s\"", iter_p->label);
      if (iter + 1 < count)
	printf(" ");
    }
    printf("\n");
  }

 list_return:
  if (addr_p) {
    for (iter = 0; iter < count; iter++) {
      if (addr_p[iter].dev)
	free(addr_p[iter].dev);
      if (addr_p[iter].label)
	free(addr_p[iter].label);
    }
    free(addr_p);
  }
  return ret_val;
}

/**
 * unlbl_add - Add a static/fallback label configuration
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Add a fallback label configuration to the kernel.  Returns zero on success,
 * negative values on failure.
 *
 */
int unlbl_add(int argc, char *argv[])
{
  int ret_val;
  uint32_t iter;
  char *mask;
  uint32_t mask_iter;
  uint8_t def_flag = 0;
  nlbl_netdev dev = NULL;
  nlbl_netaddr addr;
  nlbl_secctx label;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;

  memset(&addr, 0, sizeof(addr));

  /* parse the arguments */
  iter = 0;
  while (iter < argc && argv[iter] != NULL) {
    if (strncmp(argv[iter], "interface:", 10) == 0) {
      dev = argv[iter] + 10;
    } else if (strncmp(argv[iter], "default", 7) == 0) {
      def_flag = 1;
    } else if (strncmp(argv[iter], "label:", 6) == 0) {
      label = argv[iter] + 6;
    } else if (strncmp(argv[iter], "address:", 8) == 0) {
      mask = strstr(argv[iter] + 8, "/");
      if (mask != NULL) {
	mask[0] = '\0';
	mask++;
      }
      ret_val = inet_pton(AF_INET, argv[iter] + 8, &addr.addr.v4);
      if (ret_val > 0) {
	addr.type = AF_INET;
	mask_iter = (mask ? atoi(mask) : 32);
	for (; mask_iter > 0; mask_iter--) {
	  addr.mask.v4.s_addr >>= 1;
	  addr.mask.v4.s_addr |= 0x80000000;
	}
	addr.mask.v4.s_addr = htonl(addr.mask.v4.s_addr);
      } else {
	ret_val = inet_pton(AF_INET6, argv[iter] + 8, &addr.addr.v6);
	if (ret_val > 0) {
	  uint32_t tmp_iter;
	  addr.type = AF_INET6;
	  mask_iter = (mask ? atoi(mask) : 128);
	  for (tmp_iter = 0; mask_iter > 0 && tmp_iter < 4; tmp_iter++) {
	    for (; mask_iter > 0 &&
		   addr.mask.v6.s6_addr32[tmp_iter] < 0xffffffff;
		 mask_iter--) {
	      addr.mask.v6.s6_addr32[tmp_iter] >>= 1;
	      addr.mask.v6.s6_addr32[tmp_iter] |= 0x80000000;
	    }
	    addr.mask.v6.s6_addr32[tmp_iter] =
	      htonl(addr.mask.v6.s6_addr32[tmp_iter]);
	  }
	}
      }
      if (ret_val <= 0)
	return -EINVAL;
    }
    iter++;
  }

  /* add the mapping */
  if (def_flag)
    return nlbl_unlbl_staticadddef(NULL, &addr, label);
  else
    return nlbl_unlbl_staticadd(NULL, dev, &addr, label);
}

/**
 * unlbl_add - Delete a static/fallback label configuration
 * @argc: the number of arguments
 * @argv: the argument list
 *
 * Description:
 * Deletes a fallback label configuration to the kernel.  Returns zero on
 * success, negative values on failure.
 *
 */
int unlbl_del(int argc, char *argv[])
{
  int ret_val;
  uint32_t iter;
  char *mask;
  uint32_t mask_iter;
  uint8_t def_flag = 0;
  nlbl_netdev dev;
  nlbl_netaddr addr;

  /* sanity checks */
  if (argc <= 0 || argv == NULL || argv[0] == NULL)
    return -EINVAL;

  memset(&addr, 0, sizeof(addr));

  /* parse the arguments */
  iter = 0;
  while (iter < argc && argv[iter] != NULL) {
    if (strncmp(argv[iter], "interface:", 10) == 0) {
      dev = argv[iter] + 10;
    } else if (strncmp(argv[iter], "default", 7) == 0) {
      def_flag = 1;
    } else if (strncmp(argv[iter], "address:", 8) == 0) {
      mask = strstr(argv[iter] + 8, "/");
      if (mask != NULL) {
	mask[0] = '\0';
	mask++;
      }
      ret_val = inet_pton(AF_INET, argv[iter] + 8, &addr.addr.v4);
      if (ret_val > 0) {
	addr.type = AF_INET;
	mask_iter = (mask ? atoi(mask) : 32);
	for (; mask_iter > 0; mask_iter--) {
	  addr.mask.v4.s_addr >>= 1;
	  addr.mask.v4.s_addr |= 0x80000000;
	}
	addr.mask.v4.s_addr = htonl(addr.mask.v4.s_addr);
      } else {
	ret_val = inet_pton(AF_INET6, argv[iter] + 8, &addr.addr.v6);
	if (ret_val > 0) {
	  uint32_t tmp_iter;
	  addr.type = AF_INET6;
	  mask_iter = (mask ? atoi(mask) : 128);
	  for (tmp_iter = 0; mask_iter > 0 && tmp_iter < 4; tmp_iter++)
	    for (; mask_iter > 0 &&
		   addr.mask.v6.s6_addr32[tmp_iter] < 0xffffffff;
		 mask_iter--) {
	      addr.mask.v6.s6_addr32[tmp_iter] >>= 1;
	      addr.mask.v6.s6_addr32[tmp_iter] |= 0x80000000;
	    }
	    addr.mask.v6.s6_addr32[tmp_iter] =
	      htonl(addr.mask.v6.s6_addr32[tmp_iter]);
	}
      }
      if (ret_val <= 0)
	return -EINVAL;
    }
    iter++;
  }

  /* add the mapping */
  if (def_flag)
    return nlbl_unlbl_staticdeldef(NULL, &addr);
  else
    return nlbl_unlbl_staticdel(NULL, dev, &addr);
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
  } else if (strcmp(argv[0], "add") == 0) {
    /* add */
    ret_val = unlbl_add(argc - 1, argv + 1);
  } else if (strcmp(argv[0], "del") == 0) {
    /* del */
    ret_val = unlbl_del(argc - 1, argv + 1);
  } else {
    /* unknown request */
    ret_val = -EINVAL;
  }

  return ret_val;
}
