#include "fd6288_m2_sine_adc_capture.h"

/* BSS: neither buffer consumes a Flash initialization image. */
volatile FdM2SineAdcCapture fd_m2_sine_adc_capture;
volatile uint8_t fd_m2_sine_adc_enabled;
uint8_t fd_m2_sine_adc_pending_valid;
uint32_t fd_m2_sine_adc_origin;
FdM2SineAdcSample fd_m2_sine_adc_pending;

/* Called only AFTER control/event dispatch and injected-queue rearm. */
static void fdM2SineAdcFinishIrq(uint32_t irq_at)
{
    uint32_t before = DWT->CYCCNT;
    fdM2SineAdcCommit();
    __asm__ volatile ("" ::: "memory");
    fdM2SineAdcCost(DWT->CYCCNT - before, 0u);
    volatile FdM2SineAdcHeader *h = &fd_m2_sine_adc_capture.header;
    h->irq_count++;
    uint32_t spent = DWT->CYCCNT - irq_at;
    if (spent > h->max_irq_cycles) h->max_irq_cycles = spent;
}
