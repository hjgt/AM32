// S3-D, read only. Uses the same stopped/identity/stability gates as S3.
const fs = require('node:fs'), assert = require('node:assert/strict');
const {dump: capture} = require('./fd6288_pair_capture.cjs');
const SIZE = 21568, BANK = 7168, NONE = 0xffffffff;
const reasons = ['invalid','blank','pre','hysteresis','early','unarmed','confirmation','accepted','reported','coast'];
const events = ['', 'start','commutate','zc','timeout','desync','stop','fault','bench_stop','rev_step','rev_edge','rev_handoff','rev_decision','adc_accept'];
const states = ['empty','active','aborted','no_lock','post_handoff_loss','success'];
function decode(b) {
  assert.equal(b.length, SIZE);
  const u32 = o => b.readUInt32LE(o), u16 = o => b.readUInt16LE(o);
  const fields = ['magic','abi','variant','clock_hz','size','command','active_bank','no_lock_bank','lost_bank','success_bank','trials_seen','rejected_commands','max_finish_cycles'];
  const header = Object.fromEntries(fields.map((n,i)=>[n,u32(i*4)]));
  assert(header.magic === 0x46535731 && header.abi === 1 && [9,10,11].includes(header.variant) && header.clock_hz === 160000000 && header.size === SIZE, 'Wrong startup waveform ABI');
  assert.equal(header.active_bank, NONE, 'Active trace');
  const pins = [header.no_lock_bank,header.lost_bank,header.success_bank];
  assert(pins.every(i=>i===NONE || i<3));
  assert.equal(new Set(pins.filter(i=>i!==NONE)).size,pins.filter(i=>i!==NONE).length);
  const banks = [];
  for(let k=0;k<3;k++) {
    const base=64+k*BANK;
    const names=['state','trial_id','started','ended','reason','final_stage','handoff_at','samples_seen','samples_stored','events','no_fresh','accepted','closed_zc','timeouts','max_sample_cycles','max_event_cycles','last_sample_at'];
    const h=Object.fromEntries(names.map((n,i)=>[n,u32(base+4*i)]));
    assert([0,2,3,4,5].includes(h.state),'Active or invalid bank');
    if(!h.state) {assert(!pins.includes(k)); continue;}
    assert(h.trial_id>0 && h.trial_id<=header.trials_seen && h.final_stage<=4);
    assert(h.ended<=210000000 && h.samples_stored<=h.samples_seen && h.samples_seen<1000000 && h.events<1000000);
    assert(h.handoff_at===NONE || h.handoff_at<=h.ended);
    h.decisions=Object.fromEntries(reasons.map((r,i)=>[r,u32(base+68+4*i)]));
    assert.equal(Object.values(h.decisions).reduce((a,v)=>a+v,0),h.samples_seen);
    assert.equal(h.accepted,h.decisions.accepted);
    if(h.state===3) assert(h.reason===8 && h.final_stage!==4 && h.handoff_at===NONE);
    if(h.state===4) assert(h.reason===9 && h.final_stage===4 && h.handoff_at!==NONE);
    // Policy timestamp precedes logging timestamp slightly; allow <=1 ms skew.
    if(h.state===5) assert(h.reason===1 && h.final_stage===4 && h.handoff_at!==NONE && h.ended-h.handoff_at>=159840000);
    for(let j=0;j<3;j++) if(pins[j]===k) assert.equal(h.state,j+3);
    function ring(total, count, offset, reader) {
      const records=[];
      for(let n=Math.max(0,total-count);n<total;n++) {
        const o=base+offset+(n%count)*20, at=u32(o);
        assert(at<=h.ended);
        if(records.length) assert(at>=records.at(-1).at_cycles);
        records.push({sequence:n+1,at_cycles:at,at_us:at/160,...reader(o)});
      }
      return {overwritten:Math.max(0,total-count),records};
    }
    const samples=ring(h.samples_stored,256,128,o=>{
      const step=b[o+16], flags=b[o+17], reason=b[o+18], state=b[o+19];
      assert(step>=1 && step<=6 && flags<32 && reason<10 && (state&7)>=2 && (state&7)<=4);
      const adc=[u16(o+4),u16(o+6),u16(o+8)]; assert(adc.every(x=>x<=4095));
      const neutral=Math.floor(adc.reduce((a,v)=>a+v,0)/3);
      const phase=[0,2,0,1,2,0,1][step], margin=adc[phase]-neutral;
      return {adc,margin,directed_margin:flags&2?margin:-margin,elapsed_ticks:u16(o+10),pwm:u16(o+12),trigger:u16(o+14),step,flags,reason:reasons[reason],stage:state&7,pre_seen:!!(state&8),confirmations:state>>4};
    });
    for(let i=1;i<samples.records.length;i++) assert(samples.records[i].at_cycles-samples.records[i-1].at_cycles>=16000,'Bad sample decimation');
    const ev=ring(h.events,96,5248,o=>{
      const kind=b[o+18], step=b[o+16], flags=b[o+17], stage=b[o+19];
      assert(kind>0 && kind<events.length && step>=1 && step<=6 && flags<32 && stage>=2 && stage<=4);
      const values=[u16(o+8),u16(o+10),u16(o+12)];
      if(kind===13) assert(values.every(x=>x<=4095));
      return {kind:events[kind],elapsed_ticks:u16(o+4),interval_ticks:u16(o+6),duty:u16(o+14),step,flags,stage,...(kind===13?{adc:values}:{aux:values[0],wait_ticks:values[1],input:values[2]})};
    });
    banks.push({bank:k,pinned:pins.includes(k),classification:states[h.state],header:h,samples,events:ev});
  }
  for(const i of pins) if(i!==NONE) assert(banks.some(b=>b.bank===i));
  return {header,banks,sampling_profile:header.variant>=10 ? {name:header.variant===11?'S4':'S3-DT',dtg:206,deadtime_ticks:368,sample_floor_ticks:460,minimum_on_ticks:704} : {name:'S3-D',sample_floor_ticks:162,minimum_on_ticks:406},caveats:[
    header.variant===11 ? 'S4: original 360/400 acquisition duty; once-only six 8 ms no-edge retry sectors after two 22.5 ms expiries. Any qualified edge cancels rescue. Closed floor holds 360 to 150 ms then fades to minimum by 450 ms, independent of 30-edge threshold. Native missing-feedback count includes all retries, so enabled protection can stop earlier. Bench only.' : 'S3 acquisition/power unchanged. Variant 10 corrects ADC window for fixed DTG=0xCE; variant 9 keeps original window.',
    'Software-consumed sequential ADC data, not simultaneous scope measurements or rotor direction.',
    'Tail only: 256 samples spaced >=100 us; 96 events include every accepted ADC sample until overwritten. No guarantee of full startup history. Overwritten counts and timestamps are explicit.',
    'When 256 samples are retained, their timestamp span is >=25.5 ms. Short acquisitions or missing fresh samples can leave fewer records; inspect actual span and final sample-to-stop gap.',
    'First no-lock, first post-handoff loss and first full observation are pinned independently; operator-zero and safety stops remain unpinned. Last 16 trial summaries are separate.',
    'PWM/trigger fields read preload registers at consumption, not latched compare values at ADC trigger. Gate/ADC pin analog correctness needs external measurement if software traces remain ambiguous.',
    'Cycle maxima measure inner logger only, not full ISR. No physical protection/flight validation.'
  ]};
}
const format={mode:'wave',symbol:'fd_startup_wave',size:SIZE,decode};
const dump=(elf,out,stopped,connection)=>capture(elf,out,stopped,connection,format);
module.exports={decode,dump,SIZE,BANK,NONE};
if(require.main===module) (async()=>{
  const a=process.argv.slice(2);
  if(a[0]==='decode') console.log(JSON.stringify(decode(fs.readFileSync(a[1])),null,2));
  else if(a[0]==='dump') await dump(a[1],a[2],a.includes('--stopped'));
  else throw Error('decode S3D.bin | dump MATCHING.elf NEW.bin --stopped');
})().catch(e=>{console.error(e.message);process.exitCode=1;});
