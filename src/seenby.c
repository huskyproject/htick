#include <seenby.h>

int seenbyComp ( s_ticfile *tic, s_addr Aka)
{
   int i;

   for (i=0; i < tic->anzseenby; i++) {
       if (addrComp (tic->seenby[i], Aka) == 0) {
          return 0;
       }
   }
   return !0;
}