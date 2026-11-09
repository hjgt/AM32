const assert=require('node:assert/strict');
const SIZE=168, NONE=0xffffffff;
function decode(b){
 assert(b.length>=48&&b.length<=288,'Retry summary size');
 const names=['magic','abi','trial','attempts','failures','stage','confidence','stop_reason','successful_attempt','finished_at','max_attempts','total_limit'];
 const h=Object.fromEntries(names.map((n,i)=>[n,b.readUInt32LE(i*4)]));
 assert.equal(h.magic,0x46535231);assert([1,2].includes(h.abi));
 assert(h.max_attempts>=1&&h.max_attempts<=6);assert.equal(b.length,48+40*h.max_attempts);
 const attemptLimitMs=(h.total_limit/160000-(h.max_attempts-1)*100)/h.max_attempts;
 assert([430,630].includes(attemptLimitMs),'Unexpected attempt time limit');
 if(h.abi===1){assert.equal(h.max_attempts,3);assert.equal(attemptLimitMs,630);}
 const secondAlignMs=attemptLimitMs-330;
 assert(h.trial>0,'No trial recorded');
 assert(h.attempts>=1&&h.attempts<=h.max_attempts&&h.failures<=h.attempts);
 assert(h.stage<=5&&h.confidence<=6&&h.stop_reason<=14);
 assert(h.successful_attempt<=h.attempts);
 const fields=['begin_at','launch_at','first_edge_at','last_edge_at','qualified_edges','fail_at','off_at','end_at','failure_kind','handoff_at'];
 const kinds=['none','feedback_sector_timeout','attempt_time_limit','whole_start_time_limit'];
 const ms=n=>n===NONE?null:n/160000;
 const attempts=[];
 for(let i=0;i<h.attempts;i++){
  const raw=Object.fromEntries(fields.map((n,j)=>[n,b.readUInt32LE(48+40*i+4*j)]));
  assert(raw.qualified_edges<=6&&raw.failure_kind<=3);
  for(const n of fields.filter(n=>n.endsWith('_at')))assert(raw[n]===NONE||raw[n]<=h.total_limit+16000000,'Implausible startup age: '+n);
  assert(raw.begin_at!==NONE);
  if(raw.launch_at!==NONE)assert(raw.launch_at>=raw.begin_at);
  if(raw.first_edge_at!==NONE){assert(raw.launch_at!==NONE&&raw.first_edge_at>=raw.launch_at);assert(raw.last_edge_at>=raw.first_edge_at);}
  if(raw.handoff_at!==NONE)assert(raw.qualified_edges===6&&raw.handoff_at>=raw.last_edge_at);
  if(raw.fail_at!==NONE)assert(raw.failure_kind!==0);
  const row={attempt:i+1};for(const n of fields)if(n.endsWith('_at'))row[n.replace('_at','_ms')]=ms(raw[n]);
  row.qualified_edges=raw.qualified_edges;row.failure_kind=kinds[raw.failure_kind];
  attempts.push(row);
 }
 return {trial:h.trial,attempts_started:h.attempts,failed_attempts:h.failures,
  stage:h.stage,confidence:h.confidence,stop_reason:h.stop_reason,
  successful_attempt:h.successful_attempt||null,startup_finished_ms:ms(h.finished_at),
  total_limit_ms:ms(h.total_limit),second_alignment_ms:secondAlignMs,max_retries:h.max_attempts-1,attempts,
  caveats:[`Latest throttle episode;${h.max_attempts} attempts total including the first. Accepted events are ADC-qualified feedback, not measured rotor angle/direction.`,'Times are software DWT observations, not oscilloscope timestamps. Successful startup timing freezes at handoff; stop_reason may describe a later operator stop/runtime loss.',`First30ms+second${secondAlignMs}ms positioning repeats after100ms full bridge-off. Normal feedback is adopted; no forced first-vector dwell or early-event re-kick. Runtime feedback loss stops without restart.`]};
}
module.exports={SIZE,decode};
