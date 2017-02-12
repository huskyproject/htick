/*****************************************************************************
 * HTICK --- FTN Ticker / Request Processor
 *****************************************************************************
 * Copyright (C) 1999 by
 *
 * Gabriel Plutzar
 *
 * Fido:     2:31/1
 * Internet: gabriel@hit.priv.at
 *
 * Vienna, Austria, Europe
 *
 * This file is part of HTICK, which is based on HPT by Matthias Tichy,
 * 2:2432/605.14 2:2433/1245, mtt@tichy.de
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#if (defined(__EMX__) || defined(__MINGW32__)) && defined(__NT__)
/* we can't include windows.h for prevent conflicts: send() & other */
# define CharToOem CharToOemA
#endif

/*  compiler.h */
#include <huskylib/compiler.h>

#ifdef HAS_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAS_IO_H
# include <io.h>
#endif

#ifdef __OS2__
# define INCL_DOSPROCESS
# define INCL_DOSERRORS
# include <os2.h>
#endif

/* smapi */
#include <smapi/msgapi.h>
#include <huskylib/huskylib.h>

/* fidoconf */
#include <fidoconf/fidoconf.h>
#include <fidoconf/common.h>
#include <huskylib/log.h>
#include <huskylib/recode.h>
#include <huskylib/xstr.h>
#include <huskylib/ftnaddr.h>

/* areafix library */
#include <areafix/areafix.h>
#include <areafix/afglobal.h>

/* htick */
#include <htick.h>
#include <global.h>
#include "version.h"
#include <toss.h>
#include <scan.h>
#include <hatch.h>
#include <filelist.h>
#include <htickafix.h>
#include "report.h"

int processHatchParams( int i, int argc, char **argv )
{

  char *basename, *extdelim;

  if( argc - i < 2 )
  {
    fprintf( stderr, "Insufficient number of arguments\n" );
    return 0;
  }

  cmHatch = 1;
  hatchInfo = scalloc( sizeof( s_ticfile ), 1 );
  basename = ( argv[i] != NULL ) ? argv[i++] : "";
  hatchInfo->file = sstrdup( basename );
  hatchInfo->anzdesc = 1;
  hatchInfo->desc =
      srealloc( hatchInfo->desc, ( hatchInfo->anzdesc ) * sizeof( &hatchInfo->desc ) );
  hatchInfo->desc[0] = sstrdup( "-- description missing --" );

  /*  Check filename for 8.3, warn if not */
  basename = strrchr( hatchInfo->file, PATH_DELIM );
  if( basename == NULL )
    basename = hatchInfo->file;
  else
    basename++;
  if( ( extdelim = strchr( basename, '.' ) ) == NULL )
    extdelim = basename + strlen( basename );

  if( extdelim - basename > 8 || strlen( extdelim ) > 4 )
  {
    if( !quiet )
      fprintf( stderr, "Warning: hatching file with non-8.3 name!\n" );
  }
  basename = ( argv[i] != NULL ) ? argv[i++] : "";
  hatchInfo->area = sstrdup( basename );

  if( argc - i == 0 )
  {
    return 1;
  }
  if( stricmp( argv[i], "replace" ) == 0 )
  {
    if( ( i < argc - 1 ) && ( stricmp( argv[i + 1], "desc" ) != 0 ) )
    {
      i++;
      hatchInfo->replaces = sstrdup( argv[i] );
    }
    else
    {
      basename = strrchr( hatchInfo->file, PATH_DELIM ) ?
          strrchr( hatchInfo->file, PATH_DELIM ) + 1 : hatchInfo->file;

      hatchInfo->replaces = sstrdup( basename );
    }
    i++;
  }
  if( stricmp( argv[i], "desc" ) != 0 )
    return 1;
  else if( argc - i < 2 )
    return 1;
  i++;
  nfree( hatchInfo->desc[0] );
  hatchInfo->desc[0] = sstrdup( argv[i] );
#ifdef __NT__
  CharToOem( hatchInfo->desc[0], hatchInfo->desc[0] );
#endif
  i++;
  if( argc - i != 0 )
  {
    hatchInfo->anzldesc = 1;
    hatchInfo->ldesc =
        srealloc( hatchInfo->ldesc, ( hatchInfo->anzldesc ) * sizeof( &hatchInfo->ldesc ) );
    hatchInfo->ldesc[0] = sstrdup( argv[i] );

#ifdef __NT__
    CharToOem( hatchInfo->ldesc[0], hatchInfo->ldesc[0] );
#endif

  }
  return 1;
}

void start_help( void )
{
  printf( "%s\n", versionStr );
  printf( "\nUsage: htick [options] <command>\n"
          "Options: -q              Quiet mode (display only urgent messages to console)\n"
          "         -c <conf-file>  Specify an alternative configuration file\n"
          "Commands:\n"
          " toss [annfecho <file>]  Do tossing. [Announce new fileechos in text file]\n"
          " toss -b                 Toss bad tics\n"
          " scan                    Scan netmail area for mails to filefix and process\n"
          "                         filefix commands contained inside the mails\n"
          " ffix <addr> <command>   Process filefix command for address from command line\n"
          " ffix! <addr> <command>  The same as 'ffix' and notify the link of the \n"
          "                         processed filefix command\n"
          " relink <pattern> <addr> Refresh subscription for fileareas matching pattern\n"
          " relink -f [file] <addr> Refresh subscription for fileareas listed in the file\n"
          " resubscribe <pattern> <fromaddr> <toaddr> - move subscription of areas\n"
          "                         matching the pattern from one link to another\n"
          " resubscribe -f [file] <fromaddr> <toaddr> - move subscription of areas\n"
          "                         matching area patterns listed in this file with one\n"
          "                         pattern on a line from one link to another\n"
          " clean                   Clean passthrough dir and old files in fileechos\n"
          " announce                Announce new files\n"
          " send <file> <filearea> <address> Send file from filearea to address\n"
          " hatch <file> <area> [replace [<filemask>]] [desc <desc> [<ldesc>]]\n"
          "                         Hatch file into area using description for file,\n"
          "                         if \"replace\" is present, add replace field to TIC;\n"
          "                         if <filemask> is not present then put <file> in field;\n"
          "                         if \"desc\" is present, add description(s);\n"
          "                         a short one line <desc> and a long <ldesc>.\n"
          "                         One may use the following as <desc>:\n"
          "                           @@BBS to use first line from files.bbs\n"
          "                           @@DIZ to use 1st line from the contained file_id.diz\n"
          "                           @@<file> to use first line from <file>\n"
          "                           @BBS to load from files.bbs\n"
          "                           @DIZ to load from the contained file_id.diz\n"
          "                           @<file> to load from <file>\n"
          "                         The last three items may be used as <ldesc>.\n"
          " filelist <file> [<dirlist>] Generate filelist which includes all files in base\n"
          "                             <dirlist> - list of paths to files from filelist\n" );
}

e_exitCode processCommandLine( int argc, char **argv )
{
  int i = 0;
  e_exitCode rc = ex_OK;

  if( argc == 1 )
  {
    start_help(  );
    return ex_Help;
  }

  while( i < argc - 1 )
  {
    i++;
    if( !strcmp( argv[i], "-h" ) )
    {
      start_help(  );
      return ex_Help;
    }
    if( !strcmp( argv[i], "-v" ) )
    {
      printf( "%s", versionStr );
      return ex_Help;
    }
    if( !strcmp( argv[i], "-q" ) )
    {
      quiet = 1;
      continue;
    }
    if( !strcmp( argv[i], "-c" ) )
    {
      i++;
      if( argv[i] != NULL )
        xstrcat( &cfgFile, argv[i] );
      else
	  {
        fprintf(stderr, "Parameter missing after \"%s\"!\n", argv[i - 1]);
		rc = ex_Error;
	  }
      continue;
    }
    if( stricmp( argv[i], "toss" ) == 0 )
    {
      cmToss = 1;
      if( i < argc - 1 )
      {
        if( stricmp( argv[i + 1], "-b" ) == 0 )
        {
          cmToss = 2;
          i++;
        }
      }
      continue;
    }
    else if( stricmp( argv[i], "scan" ) == 0 )
    {
      cmScan = 1;
      continue;
    }
    else if( stricmp( argv[i], "hatch" ) == 0 )
    {
      processHatchParams( i + 1, argc, argv );
      break;
    }
    else if( stricmp( argv[i], "ffix" ) == 0 )
    {
      if( i < argc - 1 )
      {
        i++;
        parseFtnAddrZS( argv[i], &afixAddr );
        if( i < argc - 1 )
        {
          i++;
          xstrcat( &afixCmd, argv[i] );
        }
        else
		{
          fprintf(stderr, "Parameter missing after \"%s\"!\n", argv[i]);
		  rc = ex_Error;
		}
      }
      cmAfix = 1;
      continue;
    }
    else if( stricmp( argv[i], "ffix!" ) == 0 )
    {
      if( i < argc - 1 )
      {
        i++;
        parseFtnAddrZS( argv[i], &afixAddr );
        if( i < argc - 1 )
        {
          i++;
          xstrcat( &afixCmd, argv[i] );
        }
        else
		{
          fprintf(stderr, "Parameter missing after \"%s\"!\n", argv[i]);
		  rc = ex_Error;
		}
      }
      cmNotifyLink = 1;
      cmAfix = 1;
      continue;
    }
    else if( stricmp( argv[i], "relink" ) == 0 )
    {
      if( i < argc - 1 )
      {
        i++;
        xstrcat( &relinkPattern, argv[i] );
        if( i < argc - 1 )
        {
          i++;
          parseFtnAddrZS( argv[i], &relinkFromAddr );
          cmRelink = modeRelink;
        }
        else
		{
          fprintf(stderr, "Address missing after \"%s\"!\n", argv[i]);
		  rc = ex_Error;
		}
      }
      else
	  {
        fprintf(stderr, "Pattern missing after \"%s\"!\n", argv[i]);
		rc = ex_Error;
	  }
      continue;
    }
    else if( stricmp( argv[i], "resubscribe" ) == 0 )
    {
      if (i < argc-1)
      {
        i++;
        if (stricmp(argv[i], "-f") == 0)
        {
          if (i < argc-1)
          {
            i++;
            if (fexist(argv[i]))
            {
              if (fsize(argv[i]) == 0)
			  {
                fprintf(stderr, "File \"%s\" is empty\n", argv[i]);
				rc = ex_Error;
              }
              xstrcat(&resubscribePatternFile, argv[i]);
              cmRelink = modeResubsribeWithFile;
            }
            else
            {
              fprintf(stderr, "File \"%s\" does not exist\n", argv[i]);
			  rc = ex_Error;
            }
          }
          else
          {
            fprintf(stderr, "Path missing after -f\n");
			rc = ex_Error;
          }
        }
        else
        {
          xstrcat(&relinkPattern, argv[i]);
          cmRelink = modeResubsribeWithPattern;
        }
        if (i < argc-1)
        {
          i++;
          parseFtnAddrZS(argv[i], &relinkFromAddr);
          if (i < argc-1)
          {
            i++;
            parseFtnAddrZS(argv[i], &relinkToAddr);
          }
          else
          {
            fprintf(stderr, "Address missing after \"%s\"!\n", argv[i]);
            rc = ex_Error;
          }
        }
        else
        {
          fprintf(stderr, "Address missing after \"%s\"!\n", argv[i]);
          rc = ex_Error;
        }
      }
      else
      {
        fprintf(stderr, "Pattern missing after \"%s\"!\n", argv[i]);
        rc = ex_Error;
      }
    }
    else if( stricmp( argv[i], "send" ) == 0 )
    {
      cmSend = 1;
      i++;
      strcpy( sendfile, ( argv[i] != NULL ) ? argv[i++] : "" );
      strcpy( sendarea, ( argv[i] != NULL ) ? argv[i++] : "" );
      strcpy( sendaddr, ( argv[i] != NULL ) ? argv[i] : "" );
      continue;
    }
    else if( stricmp( argv[i], "filelist" ) == 0 )
    {
      cmFlist = 1;
      if( i < argc - 1 )
      {
        i++;
        flistfile = sstrdup( argv[i] );
        if( i < argc - 1 )
        {
          i++;
          dlistfile = sstrdup( argv[i] );
        }
      }
      continue;
    }
    else if( stricmp( argv[i], "announce" ) == 0 )
    {
      cmAnnounce = 1;
      continue;
    }
    else if( stricmp( argv[i], "annfecho" ) == 0 )
    {
      if( i < argc - 1 )
      {
        i++;
        cmAnnNewFileecho = 1;
        strcpy( announcenewfileecho, argv[i] );
      }
      else
      {
        fprintf(stderr, "Insufficient number of arguments\n");
		rc = ex_Error;
      }
      continue;
    }
    else if( stricmp( argv[i], "clean" ) == 0 )
    {
      cmClean = 1;
      continue;
    }
    else
    {
      fprintf(stderr, "Unrecognized command line option %s!\n", argv[i]);
      rc = ex_Error;
    }

  } /* endwhile */
  return rc;
}

void processConfig(  )
{
  char *buff = NULL;
  int exitflag = 0;

  setvar( "module", "htick" );
  SetAppModule( M_HTICK );
  config = readConfig( cfgFile );
  if( NULL == config )
  {
    nfree( cfgFile );
    fprintf( stderr, "Config file not found\n" );
    exit( 1 );
  };


  /* open Logfile */

  initLog( config->logFileDir, config->logEchoToScreen, config->loglevels,
           config->screenloglevels );
  setLogDateFormat( config->logDateFormat );
  htick_log = openLog( LogFileName, versionStr );       /* if failed: openLog() prints a message to stderr */
  if( htick_log && quiet )
    htick_log->logEcho = 0;

  if( !sstrlen( config->inbound ) && !sstrlen( config->localInbound )
      && !sstrlen( config->protInbound ) )
  {
    w_log( LL_CRIT, "You must set inbound, protInbound or localInbound in fidoconfig first" );
    exitflag = 1;
  }
  if( !sstrlen( config->outbound ) )
  {
    w_log( LL_CRIT, "You must set outbound in fidoconfig first" );
    exitflag = 1;
  }
  if( config->addrCount == 0 )
  {
    w_log( LL_CRIT, "At least one addr must be defined" );
    exitflag = 1;
  }
  if( config->linkCount == 0 )
  {
    w_log( LL_CRIT, "At least one link must be specified" );
    exitflag = 1;
  }
  if( config->fileAreaBaseDir == NULL )
  {
    w_log( LL_CRIT, "You must set FileAreaBaseDir in fidoconfig first" );
    exitflag = 1;
  }
  if( config->passFileAreaDir == NULL )
  {
    w_log( LL_CRIT, "You must set PassFileAreaDir in fidoconfig first" );
    exitflag = 1;
  }
  if( cmAnnounce && config->announceSpool == NULL )
  {
    w_log( LL_CRIT, "You must set AnnounceSpool in fidoconfig first" );
    exitflag = 1;
  }
  if( config->MaxTicLineLength && config->MaxTicLineLength < 80 )
  {
    w_log( LL_CRIT, "Parameter MaxTicLineLength (%d) in fidoconfig must be 0 or >80\n",
           config->MaxTicLineLength );
    exitflag = 1;
  }

  if( exitflag )
  {
    exit_htick( "Error(s) is found in config file, please run tparser and analize it's output.",
                1 );
  }


  if( config->lockfile )
  {
    lock_fd = lockFile( config->lockfile, config->advisoryLock );
    if( lock_fd < 0 )
    {
      disposeConfig( config );
      exit( EX_CANTCREAT );
    }
  }

  w_log( LL_START, "Start" );

  nfree( buff );

  if( config->busyFileDir == NULL )
  {
    config->busyFileDir = ( char * )smalloc( strlen( config->outbound ) + 10 );
    strcpy( config->busyFileDir, config->outbound );
    sprintf( config->busyFileDir + strlen( config->outbound ), "busy.htk%c", PATH_DELIM );
  }
  if( config->ticOutbound == NULL )
    config->ticOutbound = sstrdup( config->passFileAreaDir );
}

int main( int argc, char **argv )
{
  struct _minf m;
  int rc = 0;
  e_exitCode res;

  versionStr = GenVersionStr( "htick", VER_MAJOR, VER_MINOR, VER_PATCH, VER_BRANCH, cvs_date );

  res = processCommandLine( argc, argv );
  if( res == ex_Error )
    return EX_USAGE;
  else if( res == ex_Help )
    return EX_OK;
  processConfig(  );

  if( !( config->netMailAreas ) )
    w_log( LL_CRIT, "Netmailarea not defined, filefix not allowed!" );

  /* init areafix */
  if( !init_htickafix(  ) )
    exit_htick( "Can't init Areafix library", 1 );

  /*  init SMAPI */
  m.req_version = 0;
  m.def_zone = config->addr[0].zone;
  if( MsgOpenApi( &m ) != 0 )
  {
    exit_htick( "MsgApiOpen Error", 1 );
  }                             /*endif */

  /*  load recoding tables */
  initCharsets(  );
  getctabs( config->intab, config->outtab );
#ifdef __NT__
  SetFileApisToOEM(  );
#endif

  checkTmpDir(  );

  if( cmScan )
    scan(  );
  if( cmToss )
    toss(  );
  if( cmHatch )
    rc = hatch(  );
  if( cmSend )
    send( sendfile, sendarea, sendaddr );
  if( cmFlist )
    filelist(  );
  if( cmClean )
    Purge(  );
  if( cmAfix && config->netMailAreas )
    ffix( afixAddr, afixCmd );
  if( cmAnnounce && config->announceSpool != NULL )
    report(  );

  if (cmRelink != modeNone)
  {
    int ret;
    FILE *f;
    char *line, *fromCmd = NULL, *toCmd = NULL, *toPrint = NULL;
    unsigned int count = 0;
    
    w_log(LL_START, "%s has started", cmRelink == modeRelink ? "Relinking" : "Resubscribing");
    if (cmRelink != modeResubsribeWithFile)
    {
      /* modeRelink or modeResubsribeWithPattern */
      ret = relink(cmRelink, relinkPattern, relinkFromAddr, relinkToAddr, &fromCmd, &toCmd, &count);
      nfree(relinkPattern);
      if (ret)
        return 1;
    }
    else
    {
      /* modeResubsribeWithFile */
      f = fopen(resubscribePatternFile, "r");
      if (f == NULL)
      {
        fprintf(stderr, "Cannot open file \"%s\"!\n", resubscribePatternFile);
        nfree(resubscribePatternFile);
        return 1;
      }
      else
      {
        while ((line = readLine(f)) != NULL)
        {
          xstrcat(&relinkPattern, line);
          ret = relink(cmRelink, relinkPattern, relinkFromAddr, relinkToAddr, &fromCmd, &toCmd, &count);
          nfree(relinkPattern);
          if (ret)
          {
            nfree(resubscribePatternFile);
            return 1;
          }
        }
        nfree(resubscribePatternFile);
      }
    }
    if (fromCmd)
    {
      if (cmRelink == modeRelink)
        sendRelinkMsg(cmRelink, relinkFromAddr, fromCmd, smodeSubscribe);
      else
        sendRelinkMsg(cmRelink, relinkFromAddr, fromCmd, smodeUnsubscribe);
      nfree(fromCmd);
    }
    if (toCmd)
    {
      sendRelinkMsg(cmRelink, relinkToAddr, toCmd, smodeSubscribe);
      nfree(toCmd);
    }
    xscatprintf(&toPrint, "%i ", count);
    count == 1 ? xscatprintf(&toPrint, "%s has been ", af_robot->strA) : xscatprintf(&toPrint, "%ses have been", af_robot->strA);
    w_log(LL_AREAFIX, "%s %s", toPrint, cmRelink == modeRelink ? "relinked" : "resubscribed");
    nfree(toPrint);
  }

  nfree( afixCmd );
  nfree( flistfile );
  nfree( dlistfile );
  if( hatchInfo )
  {
    disposeTic( hatchInfo );
    nfree( hatchInfo );
  }
  /*  deinit SMAPI */
  MsgCloseApi(  );

  doneCharsets(  );
  w_log( LL_STOP, "End" );
  closeLog(  );
  if( config->lockfile )
  {
    FreelockFile( config->lockfile, lock_fd );
  }
  disposeConfig( config );
  nfree( versionStr );

  return rc;
}
