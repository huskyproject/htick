/*****************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 *****************************************************************************
 * SEEN-BY lines compare routine(s)
 *
 * This file is part of HTICK, part of the Husky fidosoft project
 * http://husky.physcip.uni-stuttgart.de
 *
 * HTICK is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * HTICK is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HTICK; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************
 * $Id$
 *****************************************************************************/

#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>    
#include <fcommon.h>   
#include <global.h>    
#include <seenby.h>

int seenbyComp ( s_addr *seenby, int anzseenby, s_addr Aka)
{
   int i;

   for (i=0; i < anzseenby; i++)
      if (addrComp (seenby[i], Aka) == 0) return 0;

   return !0;
}
