#include <assert.h>
#define TARGET_STARTUP_SEQUENCE_V2
#define TARGET_STARTUP_FEEDBACK
#define TARGET_STARTUP_HANDOFF_REAL
#define TARGET_FD_STARTUP_LEVEL
#define TARGET_FD_FIRST_EDGE_500US
#include "../Inc/fd6288_startup_sequence.h"
int main(void) {
 for(unsigned wrap=0;wrap<2;wrap++) {
 FdRevPolicy s; uint32_t now=wrap?UINT32_MAX-1000u:1000u;
 fdRevBeginPolicy(&s,now);
 /* Alternating level and crossing never earns a handoff or interval. */
 for(unsigned i=0;i<12;i++) {
 now+=10000u*160u;
 assert(!fdRevStartupEvent(&s,now,8,i%2));
 assert(s.confidence==(i%2));assert(!s.measured&&!s.last_real_period);
 now+=s.delay;assert(fdRevAdvancePolicy(&s,now)==FD_REV_STEP);
 }
 /* Level interrupts an already seeded streak. */
 now+=10000u*160u;fdRevStartupEvent(&s,now,8,0);
 assert(!s.confidence&&!s.measured&&!s.last_real_period);
 now+=s.delay;assert(fdRevAdvancePolicy(&s,now)==FD_REV_STEP);
 for(unsigned i=1;i<=6;i++) {
 now+=10000u*160u;
 assert(fdRevStartupEvent(&s,now,8,1)==(i==6));assert(s.confidence==i);
 if(i==1)assert(!s.measured);else assert(s.measured>0&&s.last_real_period>0);
 now+=s.delay;assert(fdRevAdvancePolicy(&s,now)==(i==6?FD_REV_HANDOFF:FD_REV_STEP));
 }
 }
 return 0;
}
