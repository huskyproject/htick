#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hatch.h>
#include <global.h>
#include <fcommon.h>
#include <toss.h>
#include <fidoconf/log.h>
#include <fidoconf/crc.h>
#include <fidoconf/common.h>
#include <add_desc.h>
#include <fidoconf/recode.h>
#include <version.h>
#include <smapi/progprot.h>
#include <fidoconf/adcase.h>
#include <fidoconf/xstr.h>
#include <filecase.h>

void hatch()
{
    s_ticfile tic;
    s_filearea *filearea;
    struct stat stbuf;
    extern s_newfilereport **newFileReport;
    extern int newfilesCount;
    
    newFileReport = NULL;
    newfilesCount = 0;
    
    w_log( LL_INFO, "Start file hatch...");
    
    memset(&tic,0,sizeof(tic));
    
    // Exist file?
    adaptcase(hatchfile);
    if (!fexist(hatchfile)) {
        w_log(LL_ALERT,"File %s, not found",hatchfile);
        disposeTic(&tic);
        return;
    }
    
    tic.file = sstrdup(GetFilenameFromPathname(hatchfile));
    
    MakeProperCase(tic.file);
    
    tic.area = sstrdup(hatcharea);
    filearea=getFileArea(config,tic.area);
    
    if (config->outtab != NULL) recodeToTransportCharset(hatchdesc);
    tic.desc=srealloc(tic.desc,(tic.anzdesc+1)*sizeof(&tic.desc));
    tic.desc[tic.anzdesc]=sstrdup(hatchdesc);
    tic.anzdesc++;
    if (hatchReplace) tic.replaces = sstrdup(replaceMask);
    /*
    if (filearea==NULL) {
    autoCreate(tic.area,tic.from,tic.areadesc);
    filearea=getFileArea(config,tic.area);
    }
    */
    if (filearea == NULL) {
        w_log('9',"Cannot open or create File Area %s",tic.area);
        if (!quiet) fprintf(stderr,"Cannot open or create File Area %s !",tic.area);
        disposeTic(&tic);
        return;
    } 
    
    stat(hatchfile,&stbuf);
    tic.size = stbuf.st_size;
    
    tic.origin = tic.from = *filearea->useAka;
    
    // Adding crc
    tic.crc = filecrc32(hatchfile);
    
    sendToLinks(0, filearea, &tic, hatchfile);
    
    disposeTic(&tic);
}

int send(char *filename, char *area, char *addr)
//0 - OK
//1 - Passthrough filearea
//2 - filearea not found
//3 - file not found
//4 - link not found
{
    s_ticfile tic;
    s_link *link = NULL;
    s_filearea *filearea;
    //s_addr address;
    char *sendfile=NULL, *descr_file_name=NULL, *tmpfile=NULL;
    char timestr[40], *linkfilepath=NULL;
    struct stat stbuf;
    time_t acttime;
    
    w_log( LL_INFO, "Start file send (%s in %s to %s)",filename,area,addr);
    
    filearea=getFileArea(config,area);
    if (filearea == NULL) {
        if (!quiet) fprintf(stderr,"Error: Filearea not found\n");
        return 2;
    }

    link = getLink(config, addr);
    if (link == NULL) {
        if (!quiet) fprintf(stderr,"Error: Link not found\n");
        return 4;
    }
    
    memset(&tic,0,sizeof(tic));
    
    if (filearea->pass == 1)
        sendfile = sstrdup(config->passFileAreaDir);
    else
        sendfile = sstrdup(filearea->pathName);
    
    strLower(sendfile);
    _createDirectoryTree(sendfile);
    xstrcat(&sendfile,filename);
    
    // Exist file?
    adaptcase(sendfile);
    if (!fexist(sendfile)) {
        if (!quiet) fprintf(stderr,"Error: File not found\n");
        w_log('6',"File %s, not found",sendfile);
        nfree(sendfile);
        disposeTic(&tic);
        return 3;
    }
    
    tic.file = sstrdup(filename);
    
    if (filearea->sendorig) {
        xstrscat(&tmpfile,config->passFileAreaDir,tic.file,NULL);
        adaptcase(tmpfile);
        
        if (copy_file(sendfile,tmpfile)!=0) {
            adaptcase(sendfile);
            if (copy_file(sendfile,tmpfile)==0) {
                w_log('6',"Copied %s to %s",sendfile,tmpfile);
            } else {
                w_log('9',"File %s not found or not copyable",sendfile);
                disposeTic(&tic);
                nfree(sendfile);
                nfree(tmpfile);
                return(2);
            }
        } else {
            w_log('6',"Copied %s to %s",sendfile,tmpfile);
            strcpy(sendfile,tmpfile);
        }
    }
    
    tic.area = sstrdup(area);
    
    stat(sendfile,&stbuf);
    tic.size = stbuf.st_size;
    
    tic.origin = tic.from = *filearea->useAka;
    
    // Adding crc
    tic.crc = filecrc32(sendfile);
    
    xstrscat(&descr_file_name, filearea->pathName,"files.bbs",NULL);
    adaptcase(descr_file_name);
    
    getDesc(descr_file_name, tic.file, &tic);
    
    // Adding path
    time(&acttime);
    strcpy(timestr,asctime(gmtime(&acttime)));
    timestr[strlen(timestr)-1]=0;
    if (timestr[8]==' ') timestr[8]='0';
    tic.path=srealloc(tic.path,(tic.anzpath+1)*sizeof(*tic.path));
    xscatprintf(&tic.path[tic.anzpath],"%s %lu %s UTC %s",
        aka2str(*filearea->useAka), (unsigned long) time(NULL), timestr,versionStr);
    tic.anzpath++;
    
    // Adding Downlink to Seen-By
    tic.seenby=srealloc(tic.seenby,(tic.anzseenby+1)*sizeof(s_addr));
    memcpy(&tic.seenby[tic.anzseenby], &link->hisAka, sizeof(s_addr));
    tic.anzseenby++;
    
    // Forward file to

    PutFileOnLink(sendfile, &tic, link);

    disposeTic(&tic);
    nfree(sendfile);
    nfree(tmpfile);
    nfree(descr_file_name);
    return 0;
}


void PutFileOnLink(char *newticedfile, s_ticfile *tic, s_link* downlink)
{
    int   busy = 0;
    char *linkfilepath = NULL;
    char *newticfile   = NULL;
    FILE *flohandle    = NULL;

    memcpy(&tic->from,downlink->ourAka,sizeof(s_addr));
    memcpy(&tic->to,&downlink->hisAka, sizeof(s_addr));

    nfree(tic->password);
    if (downlink->ticPwd!=NULL)
        tic->password = sstrdup(downlink->ticPwd);
    
    if(createOutboundFileName(downlink,downlink->fileEchoFlavour,FLOFILE)==1)
        busy = 1;
    
    if (busy) {
        xstrcat(&linkfilepath,config->busyFileDir);
    } else {
        if (config->separateBundles) {
            xstrcat(&linkfilepath, downlink->floFile);
            sprintf(strrchr(linkfilepath, '.'), ".sep%c", PATH_DELIM);
        } else {
            xstrcat(&linkfilepath, config->ticOutbound);
        }
    }
    
    // fileBoxes support
    if (needUseFileBoxForLink(config,downlink)) {
        nfree(linkfilepath);
        if (!downlink->fileBox) 
            downlink->fileBox = makeFileBoxName (config,downlink);
        xstrcat(&linkfilepath, downlink->fileBox);
    }
    
    _createDirectoryTree(linkfilepath);
    /* Don't create TICs for everybody */
    if (!downlink->noTIC) {
        newticfile=makeUniqueDosFileName(linkfilepath,"tic",config);
        writeTic(newticfile,tic);
    }
    else  newticfile = NULL;
    
    if (needUseFileBoxForLink(config,downlink)) {
        xstrcat(&linkfilepath, tic->file);
        if (link_file(newticedfile,linkfilepath ) == 0)
        {
            if(copy_file(newticedfile,linkfilepath )==0) {
                w_log(LL_FILESENT,"Copied: %s",newticedfile);
                w_log(LL_FILESENT,"    to: %s",linkfilepath);
            } else {
                w_log('9',"File %s not found or not copyable",newticedfile);
                if (!busy) remove(downlink->bsyFile);
                nfree(downlink->bsyFile);
                nfree(downlink->floFile);
                nfree(linkfilepath);
            }
        } else {
            w_log(LL_FILESENT,"Linked: %s",newticedfile);
            w_log(LL_FILESENT,"    to: %s",linkfilepath);
        }
    } else if (!busy) {
        flohandle=fopen(downlink->floFile,"a");
        fprintf(flohandle,"%s\n",newticedfile);
        if (newticfile != NULL) fprintf(flohandle,"^%s\n",newticfile);
        fclose(flohandle);
    }
    if (!busy || needUseFileBoxForLink(config,downlink)) {                   
        
        w_log(LL_LINK,"Forwarding: %s", tic->file);
        if (newticfile != NULL) 
        w_log(LL_LINK,"  with tic: %s", GetFilenameFromPathname(newticfile));
        w_log(LL_LINK,"       for: %s", aka2str(downlink->hisAka));
        if (!busy) remove(downlink->bsyFile);
    }
    nfree(downlink->bsyFile);
    nfree(downlink->floFile);
    nfree(linkfilepath);
}