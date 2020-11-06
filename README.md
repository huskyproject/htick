# htick
[![Build Status](https://travis-ci.org/huskyproject/htick.svg?branch=master)](https://travis-ci.org/huskyproject/htick)
[![Build status](https://ci.appveyor.com/api/projects/status/5nahbqj8f91qyydc/branch/master?svg=true)](https://ci.appveyor.com/project/dukelsky/htick/branch/master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/dc9b5e03ffda4912b72d7ee2edc6a8cc)](https://www.codacy.com/app/huskyproject/htick?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=huskyproject/htick&amp;utm_campaign=Badge_Grade)

# NAME

htick - Husky File-Ticker

# SYNOPSIS

**htick \[-h\] \[-v\] \[-q\] \[-c config-file\] \[\<command\>...\]**

# DESCRIPTION

Htick is a part of Husky Fidonet software package. It handles thematic
subscriptions to files. Each such subscription forms a file area also
known as a file echo. Every file sent from a file echo to a subscribed
sysop is accompanied with a ticket file with the filename extension .tic
containing technical information which allows correct handling of the
received file.

# OPTIONS

  - **-h** - Print program name, version and usage information to screen and
exit.

  - **-v**  
    \- Print program name and version to screen and exit.

  - **-q**  
    \- Quiet mode: don't print any messages to screen

  - **-c config-file**  
    \- Specify an alternative config file

## COMMANDS

You may place several commands one after another with one exclusion:
htick will not execute any other command after **hatch**.

  **toss**
  - Read a \*.tic file and move the file to which it refers to the
    directory of the file echo specified in the .tic file. After that
    the .tic is deleted.
    
    If some errors preventing tossing are found during parsing the .tic,
    the .tic is renamed and tossing is not executed. If the .tic
    contains wrong password, it is renamed to .sec. If there is some
    error during reading the .tic, it is renamed to .acs. If the .tic
    has wrong format i.e. does not conform to specification, it is
    renamed to .bad. And if the .tic is not to us, it is renamed to
    .ntu.

  **toss -b**
  - Try to toss ticket files with file extensions .sec, .acs, .bad and
    .ntu. It is assumed that you have already fixed the errors that led
    to the renaming of the files.

  **scan**
  - Scan the *RobotsArea* for mails to filefix, or first netmail area if
    *RobotsArea* is not defined. If at least one such mail is found, the
    filefix commands contained in the mail are executed. See also the
    "FILEFIX COMMANDS" section.

  **ffix \[-f\] \[-s\] \<address\> \<filefix command\>**
  - Send the **\<filefix command\>** to our filefix from the name of
    **\<address\>**. Our filefix will send a reply to the **\<filefix
    command\>** to the **\<address\>**. All filefix commands are listed
    below in the "FILEFIX COMMANDS" section.
    
    -  **-f** \- also send the same command to the link's filefix
    
    -  **-s** \- do not send reply from our filefix to the link

  **ffix! \<address\> \<filefix command\>**
  - The same as **ffix -f \<address\> \<command\>**

  **relink \<pattern\> \<address\>**
  - Refresh subscription for the areas matching the pattern. Htick sends
    to the filefix at the **\<address\>** a list of subscription
    commands for all file areas matching the **\<pattern\>** for which
    we are already subscribed and which are not paused.

  **resubscribe \<pattern\> \<from\-address\> \<to\-address\>**
  - Move subscription for the areas matching the pattern from one link
    to another.

  **resubscribe \-f \<file\> \<from\-address\> \<to\-address\>**
  - Move subscription from one link to another of the areas matching the
    area patterns listed in this file with one pattern on a line.

  **clean**
  - Clean passthrough dir and old files in fileechos.
    
    Every .tic file refers to some filename of a file we have received
    from a file echo. "htick **clean**" collects all such filenames to
    which .tic files in the directories defined by *busyFileDir* and
    *ticOutbound* refer and then checks every not .tic file in
    *passFileAreaDir* if it is a member of the collection. All files not
    refered by any .tic are deleted.
    
    A file echo declaration may have *-p \<number of days\>* option,
    which says that the files in the file echo older than the \<number
    of days\> should be purged. So the **clean** command deletes such
    files.
    
    If *SaveTic* statements are defined and contain
    *\<DaysToKeepTics\>*, the .tic files older than *\<DaysToKeepTics\>*
    are purged.

  **announce**
  - Announce new files. For announcing to work you have to define
    *AnnounceSpool* in htick config. Announces are made to the echo
    defined in *ReportTo*. If *ReportTo* is not defined, then announces
    are sent to the first netmail area.

  **annfecho \<file\>**
  - Announce new file echos in the text file so you may process the file
    with a script.

  **send \<file\> \<filearea\> \<address\>**
  - Send the file from the filearea to the address. It will be sent
    together with a freshly generated .tic so for the system at the
    \<address\> it will look as a regular file from the file echo.

  **hatch \<file\> \<area\> \[replace \[\<filemask\>\]\] \[desc \<desc\> \[\<ldesc\>\]\]**
  - Send the file to the file area together with a generated ticket file
    (with .tic filename extension). The **hatch** command should be the
    last one if several commands are used in one htick run, that is no
    other command is executed after the **hatch**.

>   **\<file\>**
>   - \- the file to send (the parameter is mandatory);
> 
>   **\<area\>**
>   - \- the file area to which we want to send the file (the parameter
>     is mandatory). All the rest parameters are optional;
> 
>   **replace \[\<filemask\>\]**
>   - \- this option is used to replace file(s) sent to the same file
>     echo earlier. **\<filemask\>** is the mask of the files that at
>     subscriber systems will be replaced by the file we send now. If
>     the **\<filemask\>** is omitted, then the filename coinsiding with
>     the filename we send will be replaced at subscriber systems;
> 
>   **desc \<desc\> \[\<ldesc\>\]**
>   - \- this option specifies the description of the sent file,
>     **\<desc\>** is a short one-line description, and **\<ldesc\>** is
>     a multi-line description;
>     
>     You may use a string in single or double quotes as **\<desc\>**.
>     You may also use the following macros:
>     
>     - @@BBS  \- to substitute the first line from the *files.bbs* as
>         **\<desc\>**;
>     
>     - @@DIZ  \- to substitute the first line from the *file\_id.diz* as
>         **\<desc\>**;
>     
>     - @@\<file\>  \- to substitute the first line from the **\<file\>** as
>         **\<desc\>**;
>     
>     You may use the following macros as **\<ldesc\>**:
>     
>     - @BBS  \- to substitute the whole *files.bbs* file as **\<ldesc\>**;
>     
>     - @DIZ  \- to substitute the whole *file\_id.diz* file as
>         **\<ldesc\>**;
>     
>     - @<file>  \- to substitute the whole **\<file\>** as **\<ldesc\>**.
> 
> There are some recommendations for hatching files to a file echo. It
> is better to add a file with the name *file\_id.diz* containing a
> short description of what you send. The lines in the file should not
> be longer than 45 characters and the number of lines is preferably no
> more than 10. Please put the file(s) you send together with
> *file\_id.diz* in a zip archive. The archive filename MUST be in 8.3
> format. Good manners suggest that you use **desc \<desk\>** at
> sending. You may also add **\<ldesc\>** if you wish. If you want to
> resend the archive you have already sent earlier, please do not forget
> to use **replace \[\<filemask\>\]**.

  **filelist \<file\> \[\<dirlist\>\]**
  - Generate a file list and write it to the **\<file\>**. The file list
    includes all files of all not hidden file echos and files of your
    BBS. A file echo is hidden if its declaration contains **-hide**
    option. For every file echo or BBS file area the file list contains
    3 items. The first one is file area name and description. The second
    one is a list of files in which for every file the following is
    specified: filename, file size, file date, file description. The
    third item is total number of files in the file area and their total
    size. And at the end we see the total number of files in all file
    areas and their total size. **\<dirlist\>** is a file to which a
    directory list of all file areas is written.

# FILEFIX COMMANDS

Filefix is a robot supported by htick that allows handling subscription
to file areas at an uplink system for a node or at the boss node for a
point. In order to send a command to the filefix, the sysop writes a
message to it, specifying the filefix password in the subject line, and
the filefix commands in the body of the message, one command per line.
One should put **Filefix** as an addressee name and the uplink (boss
node) Fidonet address as its address. The **Filefix** name is case
insensitive. Filefix commands received by our system from downlinks and
points are handled by htick when it runs **htick scan**.

Another option of using filefix commands is **htick ffix \<address\>
\<filefix command\>**. It allows us to run a filefix command from the
name of the system with Fidonet address **\<address\>**. In this case,
the sysop with the address **\<address\>** will receive the filefix
response.

  **%list** \- send me the list of available file areas.

  **%help** \- send me the list of filefix commands.

  **%unlinked** \- send me the list of not linked file areas.

  **%linked** \- send me the list of linked file areas.

  **%query** \- the same as **%linked**

  **%avail** \- send me the list of file areas available at your uplinks.

  **%pause** \- temporarily stop sending me files from file echos. Information
    about the subscription is not lost.

  **%resume** \- resume sending me files from file echos stopped previously by the
    **%pause** command.

  **%resend \<file\> \<file area\>** \- send me the **\<file\>** from **\<file area\>**.

  **+\<file area mask\>** \- subscribe me to the file areas matched by the **\<file area
    mask\>**.

  **\<file area mask\>** \- the same as **+\<file area mask\>**.

  **\-\<file area mask\>** \- unsubscribe me from the file areas matched by the **\<file area
    mask\>**.

# EXIT STATUS

Htick returns 0 in case of success, 1 if help was printed and 2 in case
of an error.

# EXAMPLES

Send the file HCL70401.zip to *husky* file echo accompanied by one-line
description from *file\_id.diz*.

> htick hatch HCL70401.zip husky desc @@DIZ

Send the file HCL70401.zip to *husky* file echo accompanied by both
one-line description from *file\_id.diz* and multi-line description from
there.

> htick hatch HCL70401.zip husky desc @@DIZ @DIZ

Send the file HCL70405.zip to *husky* file echo replacing the file
HCL70401.zip accompanied by both one-line description from
*file\_id.diz* and multi-line description from there.

> htick hatch HCL70405.zip husky replace HCL70401.zip desc @@DIZ @DIZ

# SEE ALSO

info fidoconf, info htick
