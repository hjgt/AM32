#include <assert.h>
#include <stdio.h>
#include "../Inc/fd6288_pin_scan.h"
static FdPinScan s;
int main(void){
 assert(sizeof(FdPinScanRecord)==24&&sizeof(FdPinScan)==12328);
 FdPinScanRecord r={.cycles=0xfffffff0u,.value={1,2,3,4,5,6}};
 for(unsigned i=0;i<600;i++)fdPinScanPush(&s,&r);
 assert(s.frozen&&s.count==512&&!s.trial);
 r.running=1;fdPinScanPush(&s,&r);
 assert(!s.frozen&&s.count==1&&s.trial==1);
 r.cycles=20;fdPinScanPush(&s,&r);
 assert(s.records[1].cycles==20&&s.count==2);
 r.running=0;fdPinScanPush(&s,&r);
 assert(s.frozen&&s.count==2);
 fdPinScanPush(&s,&r);assert(s.count==2);
 r.running=1;fdPinScanPush(&s,&r);assert(s.trial==2&&s.count==1);
 for(unsigned i=0;i<600;i++)fdPinScanPush(&s,&r);
 assert(s.frozen&&s.count==512&&s.trial==2);
 puts("PASS bounded pin capture: static/start/stop/restart/full/wrap/ABI");
}
