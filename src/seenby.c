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
