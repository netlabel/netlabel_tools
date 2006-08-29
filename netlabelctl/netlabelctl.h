/*
 * Header file for the NetLabel Control Utility
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

#ifndef _NETLABELCTL_H
#define _NETLABELCTL_H

/* option variables */
extern unsigned int opt_verbose;
extern unsigned int opt_timeout;
extern unsigned int opt_pretty;

/* module entry points */
typedef int main_function_t(int argc, char *argv[]);
int mgmt_main(int argc, char *argv[]);
int map_main(int argc, char *argv[]);
int unlbl_main(int argc, char *argv[]);
int cipsov4_main(int argc, char *argv[]);

#endif
