/*
 * NetLabel Library Field Functions
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

#ifndef _FIELDS_H_
#define _FIELDS_H_

void nlbl_put_hdr(nlbl_data *buffer,
		  const unsigned int msg_type,
		  const unsigned short msg_len,
		  const unsigned short msg_flags,
		  const unsigned int msg_pid,
		  const unsigned int msg_seq);
void nlbl_put_genlhdr(nlbl_data *buffer, unsigned int cmd);
void nlbl_putinc_hdr(nlbl_data **buffer,
		     const unsigned int msg_type,
		     const unsigned short msg_len,
		     const unsigned short msg_flags,
		     const unsigned int msg_pid,
		     const unsigned int msg_seq);
void nlbl_putinc_genlhdr(nlbl_data **buffer, unsigned int cmd);

#endif
