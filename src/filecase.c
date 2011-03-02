/*****************************************************************************
 * AreaFix for HPT (FTN NetMail/EchoMail Tosser)
 *****************************************************************************
 * Copyright (C) 2000
 *
 * Lev Serebryakov
 *
 * Fido:     2:5030/661
 * Internet: lev@serebryakov.spb.ru
 * St.Petersburg, Russia
 *
 * This file is part of HTICK.
 *
 * HTICK is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * HPT is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with HPT; see the file COPYING.  If not, write to the Free
 * Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *****************************************************************************
 * $Id$
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

/*  compiler.h */
#include <huskylib/compiler.h>

#ifdef HAS_UNISTD_H
# include <unistd.h>
#endif

/* smapi */

/* fidoconf */
#include <fidoconf/common.h>

/* htick */
#include <global.h>
#include <filecase.h>

static int dosallowin83( int c )
{
  static char dos_allow[] = "!@#$%^&()~`'-_{}.";

  if( ( c >= 'a' && c <= 'z' ) ||
      ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || strchr( dos_allow, c ) )
    return 1;
  return 0;
}


int isDOSLikeName( char *name )
{
  int nl, el, ec, uc, lc, f;
  char *p = name;

  if( !name )
  {
    w_log( LL_CRIT,
           __FILE__
           ":: Parameter is NULL: isDOSLikeName(%s). This is serious error in program, please report to developers.",
           name ? "name" : "NULL" );
    return 0;                   /* false */
  }

  nl = el = ec = uc = lc = 0;
  f = 1;
  while( *p )
  {
    if( !dosallowin83( *p ) )
    {
      f = 0;
      break;
    }
    if( '.' == *p )
      ec++;
    else
    {
      if( !ec )
        nl++;
      else
        el++;
      if( isalpha( *p ) )
      {
        if( isupper( *p ) )
          uc++;
        else
          lc++;
      }
    }
    p++;
  }
  return ( f && ec < 2 && el < 4 && nl < 9 && ( !lc || !uc ) );
}

char *MakeProperCase( char *name )
{
  if( !name )
  {
    w_log( LL_CRIT,
           __FILE__
           ":: Parameter is NULL: MakeProperCase(%s). This is serious error in program, please report to developers.",
           name ? "name" : "NULL" );
    return NULL;
  }

  if( isDOSLikeName( name ) )
  {
    switch ( config->convertShortNames )
    {
    case cUpper:
      return strUpper( name );
    case cLower:
      return strLower( name );
    default:
      return name;
    }
  }
  else
  {
    switch ( config->convertLongNames )
    {
    case cUpper:
      return strUpper( name );
    case cLower:
      return strLower( name );
    default:
      return name;
    }
  }
}
