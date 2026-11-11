#pragma once
#include <stdint.h>
enum { FD_PIN_SCAN_CAPACITY=512 };
typedef struct {
 uint32_t cycles;
 uint16_t value[6]; /* PA3, PA4, PA7, PB0, PA5 voltage, PA6 current */
 uint16_t gpio_a, gpio_b, gpio_f;
 uint8_t step, running;
} FdPinScanRecord;
typedef struct {
 uint32_t magic,abi,count,frozen,trial,was_running,skipped;
 uint32_t moder_a,moder_b,moder_f;
 FdPinScanRecord records[FD_PIN_SCAN_CAPACITY];
} FdPinScan;
static inline void fdPinScanPush(volatile FdPinScan *s,const FdPinScanRecord *r)
{
 if(r->running&&!s->was_running){s->count=0;s->frozen=0;s->trial++;}
 if(!r->running&&s->was_running)s->frozen=1;
 s->was_running=r->running;
 if(s->frozen)return;
 uint32_t n=s->count;
 if(n>=FD_PIN_SCAN_CAPACITY){s->frozen=1;return;}
 s->records[n]=*r;
 s->count=n+1;
 if(n+1==FD_PIN_SCAN_CAPACITY)s->frozen=1;
}
