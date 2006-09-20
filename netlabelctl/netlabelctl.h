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

/* global program name */
extern char *name_nlctl;

/* global option variables */
extern uint32_t opt_verbose;
extern uint32_t opt_timeout;
extern uint32_t opt_pretty;

/* warning/error reporting */
#define MSG_WARN(x) "%s: warning, "x,name_nlctl
#define MSG_WARN_MOD(m,x) "%s: warning[%s], "x,name_nlctl,m
#define MSG_ERR(x) "%s: error, "x,name_nlctl
#define MSG_ERR_MOD(m,x) "%s: error[%s], "x,name_nlctl,m

/* message display */
#define MSG(x) (opt_pretty?x:"")
#define MSG_V(x) (opt_verbose?x"")

/* module entry points */
typedef int main_function_t(int argc, char *argv[]);
int mgmt_main(int argc, char *argv[]);
int map_main(int argc, char *argv[]);
int unlbl_main(int argc, char *argv[]);
int cipsov4_main(int argc, char *argv[]);

#endif
