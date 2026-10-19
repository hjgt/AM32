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
  assert(header.magic === 0x46535731 && header.abi === 1 && [9,10,11,12,13,14,15,16,17,18,19,20].includes(header.variant) && header.clock_hz === 160000000 && header.size === SIZE, 'Wrong startup waveform ABI');
  const acquisition = header.variant === 15;
  const edgeWindow = header.variant >= 16;
  const lossWindow = header.variant >= 18;
  const sampleTicks = acquisition ? 192000 : 16000;
  if(acquisition) assert.equal(u32(52),sampleTicks,'Wrong acquisition spacing');
  if(edgeWindow) {assert.equal(u32(52),16000);assert.equal(u32(56),lossWindow?0:1920000);}
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
    let firstEdge = null;
    if(edgeWindow && u32(base+108)!==NONE) {
      const at=u32(base+108),packed=u32(base+124);
      if(!lossWindow) assert.equal(packed>>>24,0);
      firstEdge={at_us:at/160,adc:[u16(base+112),u16(base+114),u16(base+116)],elapsed_ticks:u16(base+118),pwm:u16(base+120),trigger:u16(base+122),step:packed&255,flags:(packed>>8)&255,stage:(packed>>>16)&255};
      assert(at<=h.ended && h.accepted>0 && firstEdge.adc.every(v=>v<=4095));
      assert(firstEdge.step>=1 && firstEdge.step<=6 && firstEdge.flags<32 && firstEdge.stage>=2 && firstEdge.stage<=4);
    } else if(edgeWindow) assert.equal(h.accepted,0,'Missing first edge');
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
    for(let i=1;i<samples.records.length;i++) assert(samples.records[i].at_cycles-samples.records[i-1].at_cycles>=sampleTicks,'Bad sample decimation');
    if(acquisition) {
      assert(h.samples_stored<=256 && !samples.overwritten);
      assert(samples.records.every(r=>r.at_cycles<=48000000 && r.stage!==4 && (h.handoff_at===NONE || r.at_cycles<=h.handoff_at)), 'Not acquisition-only samples');
    }
    if(firstEdge && !lossWindow) assert(samples.records.every(r=>r.at_cycles<=u32(base+108)+1920000),'Samples after trigger window');
    const ev=ring(h.events,96,5248,o=>{
      const kind=b[o+18], step=b[o+16], flags=b[o+17], stage=b[o+19];
      assert(kind>0 && kind<events.length && step>=1 && step<=6 && flags<32 && stage>=2 && stage<=4);
      const values=[u16(o+8),u16(o+10),u16(o+12)];
      if(kind===13) assert(values.every(x=>x<=4095));
      return {kind:events[kind],elapsed_ticks:u16(o+4),interval_ticks:u16(o+6),duty:u16(o+14),step,flags,stage,...(kind===13?{adc:values}:{aux:values[0],wait_ticks:values[1],input:values[2]})};
    });
    const freezeKind=u32(base+124)>>>24;
    if(lossWindow) assert(freezeKind<=2);
    banks.push({bank:k,pinned:pins.includes(k),classification:states[h.state],header:h,samples,events:ev,...(edgeWindow?{first_edge:firstEdge}:{}),...(lossWindow?{sample_freeze:['trial_end','first_post_feedback_timeout','handoff'][freezeKind]}:{})});
  }
  for(const i of pins) if(i!==NONE) assert(banks.some(b=>b.bank===i));
  return {header,banks,recording_profile:{mode:lossWindow?'first_feedback_loss':edgeWindow?'first_edge_window':acquisition?'acquisition_prefix':'tail',sample_spacing_us:sampleTicks/160,...(edgeWindow?{post_trigger_us:lossWindow?null:12000}:{})},sampling_profile:header.variant>=10 ? {name:header.variant===20?'S4G-SEED':header.variant===19?'S4F-PHASE':header.variant===18?'S4E-LOSS':header.variant===17?'S4E':header.variant===16?'S4D-EDGE':header.variant===15?'S4D-LOG':header.variant===14?'S4D':header.variant===13?'S4C':header.variant===12?'S4B':header.variant===11?'S4':'S3-DT',dtg:206,deadtime_ticks:368,sample_floor_ticks:460,minimum_on_ticks:704} : {name:'S3-D',sample_floor_ticks:162,minimum_on_ticks:406},caveats:[
    header.variant>=11 ? 'S4 family: original 360/400 acquisition duty; once-only six 8 ms retry sectors after two 22.5 ms expiries. Edge cancellation depends on variant below. Closed floor holds 360 to 150 ms then fades to minimum by 450 ms. Native missing-feedback count includes all retries. Bench only.' : 'S3 acquisition/power unchanged. Variant 10 corrects ADC window for fixed DTG=0xCE; variant 9 keeps original window.',
    header.variant>=12 ? 'S4B/S4C seed the ADC handoff guard with the latest real adjacent interval, not the IIR. IIR, first commutation delay and early-pulse rejection are unchanged.' : 'Original handoff guard initialization.',
    header.variant>=14 ? 'S4D/S4D-LOG keep only the remaining short-sector budget after a solitary edge during rescue. Both edges and timeouts consume sectors; a second consecutive edge cancels the remainder. No refund, new attempt or acquisition extension.' : header.variant===13 ? 'S4C retains an unused rescue after early real edges. Two consecutive missing-feedback sectors since the latest edge are required; starting rescue consumes its one-shot budget. An interrupted rescue is never resumed; no rescue after handoff. Original 300 ms acquisition deadline remains.' : 'S4/S4B cancel even an unused rescue permanently on the first real edge.',
    'Software-consumed sequential ADC data, not simultaneous scope measurements or rotor direction.',
    ...(header.variant===20 ? ['S4G-SEED additionally waits a fixed 500us after each seed edge (no trusted period yet), including after a timeout resets the streak. No blind cadence, budget, protection or handoff change.'] : []),
    ...(header.variant>=19 ? ['S4F-PHASE: acquisition edges 2..5 wait latest real interval*(1/2-min(advance64,24)/64), capped at 3ms. Seed, sixth-edge handoff, blind retry cadence and native protection unchanged. Experimental, not flight validated.'] : []),
    header.variant>=17 ? 'S4E overrides short-sector waiting after a real edge: 22.5 ms feedback window, expiry consumes budget and resumes remaining 8 ms sectors. One-second observation freezes diagnostics, NOT the motor. Later loss still stops drive but may have no waveform; final summary reason can differ from the frozen observation. Not flight-validated.' : 'Motor observation remains limited to one second after handoff.',
    lossWindow ? '100 us tail freezes on first acquisition timeout after accepted feedback, or handoff. No-edge trials retain tail; first-edge exact fields retained.' : edgeWindow ? '100 us tail freezes 12 ms after FIRST accepted ADC edge. Trigger sample preserved separately; later edges cannot retrigger. No-edge trial retains tail. Short trial/missing samples limit pre/post coverage; this is not a full acquisition waveform.' : 'No first-edge triggered window.',
    acquisition ? 'Acquisition prefix: >=1200 us spacing, up to 256 samples, no sample overwrite, stops at handoff or 300 ms. No closed-loop ADC waveform. Sub-ms crossings may fall between samples; exact accepted-edge events remain in the separate 96-event tail and may be overwritten during successful observation.' : 'Tail only: 256 samples spaced >=100 us; 96 events include every accepted ADC sample until overwritten. No guarantee of full startup history.',
    'Missing fresh samples may leave gaps; inspect actual timestamps, counts and event overwrite. Software direction flags are commands, not measured rotor direction.',
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
