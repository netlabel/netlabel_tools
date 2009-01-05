/*
 * NetLabel Control Utility, netlabelctl
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <libnetlabel.h>
#include <version.h>

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
 * Display usage information
 * @param fp the output file pointer
 *
 * Display brief usage information.
 *
 */
static void nlctl_usage_print(FILE *fp)
{
	fprintf(fp, "usage: %s [<flags>] <module> [<commands>]\n", nlctl_name);
}

/**
 * Display version information
 * @param fp the output file pointer
 *
 * Display the version string.
 *
 */
static void nlctl_ver_print(FILE *fp)
{
	fprintf(fp,
	"NetLabel Control Utility, version %s (libnetlabel %s)\n",
	VERSION_NETLABELCTL,
	NETLBL_VER_STRING);
}

/**
 * Display help information
 * @param fp the output file pointer
 *
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
	"    add default|domain:<domain> [address:<ADDR>[/<MASK>]]\n"
	"                                protocol:<protocol>[,<extra>]\n"
	"    del default|domain:<domain>\n"
	"    list\n"
	"  unlbl : Unlabeled packet handling\n"
	"    accept on|off\n"
	"    add default|interface:<DEV> address:<ADDR>[/<MASK>]\n"
	"                                label:<LABEL>\n"
	"    del default|interface:<DEV> address:<ADDR>[/<MASK>]\n"
	"    list\n"
	"  cipsov4 : CIPSO/IPv4 packet handling\n"
	"    add trans doi:<DOI> tags:<T1>,<Tn>\n"
	"            levels:<LL1>=<RL1>,<LLn>=<RLn>\n"
	"            categories:<LC1>=<RC1>,<LCn>=<RCn>\n"
	"    add pass doi:<DOI> tags:<T1>,<Tn>\n"
	"    add local doi:<DOI>\n"
	"    del doi:<DOI>\n"
	"    list [doi:<DOI>]\n"
	"\n",
	nlctl_name);
}

/**
 * Convert a errno value into a human readable string
 * @param ret_val the errno return value
 *
 * Return a pointer to a human readable string describing the error in
 * @ret_val.
 *
 */
static char *nlctl_strerror(int ret_val)
{
	char *str = NULL;

	switch (ret_val) {
	case 0:
		str = "operation succeeded";
		break;
	case EINVAL:
		str = "invalid argument or parameter";
		break;
	case ENOMEM:
		str = "out of memory";
		break;
	case ENOENT:
		str = "entry does not exist";
		break;
	case ENODATA:
		str = "no data was available";
		break;
	case EBADMSG:
		str = "bad message";
		break;
	case ENOPROTOOPT:
		str = "not supported";
		break;
	case EAGAIN:
		str = "try again";
		break;
	case ENOMSG:
		str = "no message was received";
		break;
	default:
		str = strerror(ret_val);
	}
  
	return str;
}

/**
 * Display a network address
 * @param addr the IP address to display
 *
 * Print the IP address and mask, specified in @addr, to STDIO.
 *
 */
void nlctl_addr_print(const struct nlbl_netaddr *addr)
{
	char addr_s[80];
	socklen_t addr_s_len = 80;
	struct in_addr mask4;
	struct in6_addr mask6;
	uint32_t mask_size;
	uint32_t mask_off;

	switch (addr->type) {
	case AF_INET:
		mask4.s_addr = ntohl(addr->mask.v4.s_addr);
		for (mask_size = 0; mask4.s_addr != 0; mask_size++)
			mask4.s_addr <<= 1;
		printf("%s/%u",
		       inet_ntop(AF_INET, &addr->addr.v4, addr_s, addr_s_len),
		       mask_size);
		break;
	case AF_INET6:
		for (mask_size = 0, mask_off = 0; mask_off < 4; mask_off++) {
			mask6.s6_addr32[mask_off] =
				      ntohl(addr->mask.v6.s6_addr32[mask_off]);
			while (mask6.s6_addr32[mask_off] != 0) {
				mask_size++;
				mask6.s6_addr32[mask_off] <<= 1;
			}
		}
		printf("%s/%u",
		       inet_ntop(AF_INET6, &addr->addr.v6, addr_s, addr_s_len),
		       mask_size);
		break;
	default:
		printf("UNKNOWN(%u)", addr->type);
		break;
	}
}

/**
 * Add a domain mapping to NetLabel
 * @param addr_str the IP address/mask in string format
 * @param addr the IP address/mask in native NetLabel format
 *
 * Parse the IP address/mask string into the given nlbl_netaddr structure.
 * Returns zero on success, negative values on failure.
 *
 */
int nlctl_addr_parse(char *addr_str, struct nlbl_netaddr *addr)
{
	int ret_val;
	char *mask;
	uint32_t iter_a;
	uint32_t iter_b;

	/* sanity checks */
	if (addr_str == NULL || addr_str[0] == '\0')
		return -EINVAL;

	/* separate the address mask */
	mask = strstr(addr_str, "/");
	if (mask != NULL) {
		mask[0] = '\0';
		mask++;
	}

	/* ipv4 */
	ret_val = inet_pton(AF_INET, addr_str, &addr->addr.v4);
	if (ret_val > 0) {
		addr->type = AF_INET;
		iter_a = (mask ? atoi(mask) : 32);
		for (; iter_a > 0; iter_a--) {
			addr->mask.v4.s_addr >>= 1;
			addr->mask.v4.s_addr |= 0x80000000;
		}
		addr->mask.v4.s_addr = htonl(addr->mask.v4.s_addr);
		return 0;
	}

	/* ipv6 */
	ret_val = inet_pton(AF_INET6, addr_str, &addr->addr.v6);
	if (ret_val > 0) {
		addr->type = AF_INET6;
		iter_a = (mask ? atoi(mask) : 128);
		for (iter_b = 0; iter_a > 0 && iter_b < 4; iter_b++) {
			for (; iter_a > 0 &&
			       addr->mask.v6.s6_addr32[iter_b] < 0xffffffff;
			     iter_a--) {
				addr->mask.v6.s6_addr32[iter_b] >>= 1;
				addr->mask.v6.s6_addr32[iter_b] |= 0x80000000;
			}
			addr->mask.v6.s6_addr32[iter_b] =
					htonl(addr->mask.v6.s6_addr32[iter_b]);
		}
		return 0;
	}

	return -EINVAL;
}

/*
 * main
 */
int main(int argc, char *argv[])
{
	int ret_val = RET_ERR;
	int arg_iter;
	main_function_t *module_main = NULL;
	char *module_name;

	/* save the invoked program name for use in user notifications */
	nlctl_name = strrchr(argv[0], '/');
	if (nlctl_name == NULL)
		nlctl_name = argv[0];
	else if (nlctl_name[0] == '/')
		nlctl_name += 1;
	else
		nlctl_name = strdup("unknown");

	/* get the command line arguments and module information */
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
	module_name = argv[optind];
	if (!module_name) {
		nlctl_usage_print(stderr);
		return RET_USAGE;
	}

	/* perform any setup we have to do */
	ret_val = nlbl_init();
	if (ret_val < 0) {
		fprintf(stderr,
			MSG_ERR("failed to initialize the NetLabel library\n"));
		goto exit;
	}
	nlbl_comm_timeout(opt_timeout);

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
		fprintf(stderr,
			MSG_ERR("unknown or missing module '%s'\n"),
			module_name);
		goto exit;
	}
	ret_val = module_main(argc - optind - 1, argv + optind + 1);
	if (ret_val < 0) {
		fprintf(stderr, MSG_ERR("%s\n"), nlctl_strerror(-ret_val));
		ret_val = RET_ERR;
	} else
		ret_val = RET_OK;
exit:
	nlbl_exit();
	return ret_val;
}
