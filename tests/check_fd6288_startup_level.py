from pathlib import Path
import tempfile, subprocess
root=Path(__file__).resolve().parents[1]
s=(root/'Mcu/g431/Src/comparator.c').read_text();a=s.index('#ifdef TARGET_FD_STARTUP_LEVEL\nstatic uint8_t adc_zcd_event_crossing');b=s.index('\n}\n',a)+3
stub='''#include <stdint.h>
#include <assert.h>
#define TARGET_FD_STARTUP_LEVEL
#define TARGET_FD_NO_DIAGNOSTICS
#define ADC_ZCD_HYSTERESIS_COUNTS 8
#define INTERVAL_TIMER_COUNT 100
#define FD_DECISION(x) ((void)0)
#define FD_DIAG(x) ((void)0)
static unsigned rising=1,fd_rev_acquiring=1,adc_zcd_reported,adc_zcd_before_seen,adc_zcd_confirmations,adc_zcd_earliest,adc_zcd_required_samples=2,adc_zcd_last_real_interval,adc_zcd_drive_crossings;
'''
test='''
static void reset(void){adc_zcd_event_crossing=adc_zcd_reported=adc_zcd_before_seen=adc_zcd_confirmations=0;}
int main(void){
for(int polarity=0;polarity<2;polarity++){
 rising=polarity;int post=polarity?20:-20;
 reset();fd_rev_acquiring=1;
 for(int i=0;i<6;i++)assert(!adcZcdQualifyDrive(post));
 assert(adcZcdQualifyDrive(post));assert(!adcZcdEventIsCrossing());assert(!adcZcdQualifyDrive(post));
 reset();for(int i=0;i<6;i++)assert(!adcZcdQualifyDrive(post));
 assert(!adcZcdQualifyDrive(0));
 for(int i=0;i<6;i++)assert(!adcZcdQualifyDrive(post));
 assert(!adcZcdQualifyDrive(-post));
 for(int i=0;i<6;i++)assert(!adcZcdQualifyDrive(post));
 assert(adcZcdQualifyDrive(post));assert(adcZcdEventIsCrossing());
 reset();fd_rev_acquiring=0;
 for(int i=0;i<20;i++)assert(!adcZcdQualifyDrive(post));
 assert(!adcZcdQualifyDrive(-post));assert(!adcZcdQualifyDrive(post));assert(adcZcdQualifyDrive(post));
}return 0;}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.c').write_text(stub+s[a:b]+test)
 subprocess.run(['cc','-std=c11','-Wall','-Werror',str(p/'test.c'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('Actual qualifier: both polarities, 7 groups, broken runs, one report, closed-loop pre requirement PASS')
