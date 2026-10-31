#include <assert.h>
#include <stdio.h>
#include "../Inc/fd6288_pin_sync.h"
static FdPinSync s;
int main(void){
 assert(sizeof(FdPinSyncRecord)==24&&sizeof(FdPinSync)==12320);
 FdPinSyncRecord r={.cycles=0xff000000u,.arr=6665,.flags=11};
 fdPinSyncPush(&s,&r,1,1);assert(!s.closed_seen&&!s.count);
 fdPinSyncPush(&s,&r,0,0);assert(!s.count);
 fdPinSyncPush(&s,&r,1,0);assert(s.closed_seen&&!s.count&&s.trial==1);
 r.cycles+=31999999u;fdPinSyncPush(&s,&r,1,0);assert(!s.count);
 r.cycles++;fdPinSyncPush(&s,&r,1,0);assert(s.count==1&&!(s.records[0].flags&16));
 r.cycles+=6666;fdPinSyncPush(&s,&r,1,0);assert(s.records[1].flags==27);
 r.cycles+=13332;fdPinSyncPush(&s,&r,1,0);assert(s.records[2].flags==11);
 for(unsigned i=0;i<600;i++){r.cycles+=6666;fdPinSyncPush(&s,&r,1,0);}
 assert(s.count==512&&s.frozen);
 fdPinSyncPush(&s,&r,1,1);assert(s.frozen&&!s.closed_seen);
 fdPinSyncPush(&s,&r,1,0);assert(!s.frozen&&s.count==0&&s.trial==2);
 puts("PASS sync capture: acquisition excluded, 200ms gate, wrap, cadence, capacity, restart, ABI");
}
