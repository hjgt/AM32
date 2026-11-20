// Builds exactly one target. Modes share the FIRST30 baseline, never make all.
const {spawnSync}=require('node:child_process'),path=require('node:path');
const mode=process.argv[2]||'jeos';
const firstEdge500=process.env.FD6288_FIRST_EDGE_500US==='1';
if(firstEdge500&&mode!=='alignnative')throw Error('First-edge probe requires alignnative');
const pinScan=process.env.FD6288_PIN_SCAN==='1';
const pinSync=process.env.FD6288_PIN_SYNC==='1';
const alignHold=process.env.FD6288_ALIGN_HOLD==='1';
const acqStats=process.env.FD6288_ACQ_STATS==='1';
const launchAdvance=process.env.FD6288_LAUNCH_ADVANCE==='1';
const alignSettle=process.env.FD6288_ALIGN_SETTLE==='1';
const planned=process.env.FD6288_PLANNED_ACQ==='1';
const reposition=process.env.FD6288_REPOSITION==='1';
const fullRetry=process.env.FD6288_FULL_RETRY==='1';
const retryAdjacent=process.env.FD6288_RETRY_ADJACENT==='1';
const staleRetry=process.env.FD6288_STALE_RETRY==='1';
if(staleRetry&&!retryAdjacent)throw Error('Stale retry requires adjacent retry');
if(retryAdjacent&&!fullRetry)throw Error('Adjacent retry requires full retry');
if(fullRetry&&!reposition)throw Error('Full retry requires ordered reposition');
const edgeObserve=Number(process.env.FD6288_EDGE_OBSERVE||0);
if(![0,1,2].includes(edgeObserve))throw Error('EDGE_OBSERVE must be 0, 1 or 2');
if(edgeObserve&&(!alignSettle||planned||(reposition&&edgeObserve===1)))throw Error('Edge observer requires original S4P control');
if(reposition&&(!alignSettle||planned))throw Error('Reposition requires feedback S4P baseline');
if(planned&&!alignSettle)throw Error('Planned acquisition requires S4P baseline');
if(alignSettle&&!launchAdvance)throw Error('Alignment settle requires S4O baseline');
if(launchAdvance&&(!acqStats||!alignHold))
 throw Error('Launch advance requires S4N hold and acquisition statistics');
if(acqStats&&(mode!=='alignnative'||pinScan||pinSync||process.env.FD6288_DIAGNOSTICS==='1'))
 throw Error('Acquisition stats require clean alignnative without pin scanning');
if(alignHold&&(mode!=='alignnative'||pinScan||pinSync))
 throw Error('Alignment hold requires alignnative without pin scanning');
if(pinSync&&(pinScan||mode!=='alignclean'||process.env.FD6288_DIAGNOSTICS==='1'))
 throw Error('Pin sync requires alignclean and no other recorders');
if(pinScan && (mode!=='alignclean'||process.env.FD6288_DIAGNOSTICS==='1'))
 throw Error('Pin scan requires alignclean, with all other diagnostics off');
if(!['off','async','jeos','dense','align','align360','alignstats','alignclean','alignnative'].includes(mode)) throw Error('mode: off | async | jeos | dense | align | align360 | alignstats | alignclean | alignnative');
// One-step diagnostic removal. Override with FD6288_DIAGNOSTICS=1 only
// for controlled comparison; runtime debug writes never toggle this.
const noDiagnostics=process.env.FD6288_DIAGNOSTICS==='1'?0:
    (['alignclean','alignnative'].includes(mode)?1:0);
const flags={STARTUP_TRACE:1,STARTUP_TIMING_FIX:1,STARTUP_SEQUENCE:3,
 STARTUP_PAIR_TRACE:1,STARTUP_WAVE_TRACE:1,ZCD_ACTUAL_DEADTIME:1,STARTUP_S4:1,
 STARTUP_HANDOFF_REAL:1,STARTUP_RESCUE_RETAIN:1,STARTUP_RESCUE_BRIDGE:1,
 STARTUP_ACQ_LOG:2,STARTUP_S4E:1,STARTUP_LOSS_LOG:1,STARTUP_PHASE_DELAY:1,
 STARTUP_SEED_DELAY:1,STARTUP_NOEDGE_LOG:1,STARTUP_RESCUE_RAMP:1,
 STARTUP_SPLIT_LOG:1,STARTUP_FIRST_WAIT:1,ASYNC_COMM:mode==='off'?0:1,
 ADC_JEOS:['jeos','dense','align','align360','alignstats','alignclean','alignnative'].includes(mode)?1:0,DENSE_LOG:mode==='dense'?1:0,
 DOUBLE_ALIGN:mode.startsWith('align')?1:0,QUIET_LOG:mode.startsWith('align')?1:0,
 ALIGN_BUDGET:['align360','alignstats','alignclean','alignnative'].includes(mode)?1:0,SECTOR_DIAG:mode==='alignstats'?1:0,
 NO_DIAGNOSTICS:noDiagnostics,NATIVE_EARLY:mode==='alignnative'?1:0,ALIGN_HOLD:alignHold?1:0,ACQ_STATS:acqStats?1:0,LAUNCH_ADVANCE:launchAdvance?1:0,ALIGN_SETTLE:alignSettle?1:0,PIN_SCAN:pinScan?1:0,PIN_SYNC:pinSync?1:0};
const suffix=(alignHold?'-hold':'')+(acqStats?'-acqstats':'')+(launchAdvance?'-launch':'')+(alignSettle?'-settle':'')+(pinSync?'-pinsync':pinScan?'-pinscan':process.env.FD6288_DIAGNOSTICS==='1'?'-diag':'');
flags.PLANNED_ACQ=planned?1:0;
flags.REPOSITION=reposition?1:0;
flags.FULL_RETRY=fullRetry?1:0;
flags.RETRY_ADJACENT=retryAdjacent?1:0;
flags.STALE_RETRY=staleRetry?1:0;
flags.EDGE_OBSERVE=edgeObserve;
flags.STARTUP_LEVEL=process.env.FD6288_STARTUP_LEVEL==='1'?1:0;
flags.FIRST_EDGE_500US=firstEdge500?1:0;
flags.ALIGN_SAMPLE_PAIR=process.env.FD6288_ALIGN_SAMPLE_PAIR==='1'?1:0;
if(flags.ALIGN_SAMPLE_PAIR&&edgeObserve!==2)throw Error('Alignment pair requires raw observer');
const args=['-B','-j1','CUSTOM_FD6288_G431',`OBJ=obj-startup-S4J-${mode}${suffix}${planned?'-planned':''}${reposition?'-reposition-safe':''}${fullRetry?'-fullretry':''}${retryAdjacent?'-adjacent':''}${staleRetry?'-stale-late':''}${edgeObserve===2?'-raw-edge':edgeObserve?'-edge':''}${flags.STARTUP_LEVEL?'-level':''}${firstEdge500?'-first500':''}${flags.ALIGN_SAMPLE_PAIR?'-align-pair':''}`,
 'ARM_SDK_PREFIX=arm-none-eabi-',...Object.entries(flags).map(([k,v])=>`FD6288_${k}=${v}`)];
const r=spawnSync('make',args,{cwd:path.resolve(__dirname,'../..'),stdio:'inherit'});
process.exit(r.status??1);
