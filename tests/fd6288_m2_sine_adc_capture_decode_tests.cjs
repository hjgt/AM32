// Synthetic recorder buffers only. No hardware access.
const assert=require('node:assert/strict');
const {decodeM2AdcCapture}=require('../tools/scripts/fd6288_native_sine_capture.cjs');

const size=3152,headerSize=80,capacity=128,sampleSize=24;

function fixture(count=1,state=2) {
  const buffer=Buffer.alloc(size);
  const header=[0x464d4431,1,7,count,state,160000,4,1001,4001,12,
    160,320,480,1000,80000,2,0,0,0,0];
  header.forEach((value,i)=>buffer.writeUInt32LE(value,i*4));
  for(let sequence=Math.max(0,count-capacity);sequence<count;sequence++) {
    const at=headerSize+(sequence%capacity)*sampleSize;
    buffer.writeUInt32LE((sequence+1)*160,at);
    [100,200,300,101,4001,1334,960,1250].forEach((value,i)=>{
      buffer.writeUInt16LE(value,at+4+i*2);
    });
    buffer.writeUInt8(sequence%6+1,at+20);
    buffer.writeUInt8(7,at+21);
    buffer.writeUInt8(0x3f,at+22);
    buffer.writeUInt8(0xa3,at+23);
  }
  return buffer;
}

const sample=decodeM2AdcCapture(fixture()).samples[0];
assert.deepEqual(sample,{
  at_ms:0.001,window_us:0,a:100,b:200,c:300,elapsed_us:50.5,period_us:2000.5,
  duty_ticks:1334,trigger_ticks:960,read_counter:1250,step:1,decision:7,flags:0x3f,
  rising:true,sample_valid:true,before_seen:true,reported:true,accepted:true,
  collision:true,blank_before:3,confirm_after:10
});
const decoded=decodeM2AdcCapture(fixture());
assert.equal(decoded.trial,7);
assert.equal(decoded.count,1);
assert.equal(decoded.trigger_at_ms,1);
assert.equal(decoded.first_at_ms,0.5);
assert.equal(decoded.trigger_elapsed_us,500.5);
assert.equal(decoded.trigger_period_us,2000.5);
assert.equal(decoded.max_stage_us,1);
assert.equal(decoded.max_commit_us,2);
assert.equal(decoded.max_irq_us,3);
assert.equal(decoded.irq_count,1000);
assert.equal(decoded.budget_trips,2);
assert.equal(decoded.post_remaining,12);

for(const count of [0,1,127,128,129,256,300]) {
  const capture=decodeM2AdcCapture(fixture(count));
  const retained=Math.min(count,capacity);
  assert.equal(capture.samples.length,retained);
  assert.deepEqual(capture.samples.map(value=>value.at_ms),
    Array.from({length:retained},(_,i)=>(count-retained+i+1)/1000));
  assert.deepEqual(capture.samples.map(value=>value.window_us),
    Array.from({length:retained},(_,i)=>i));
  assert.equal(capture.window_span_us,retained?retained-1:0);
}
console.log('PASS M2 ADC ring empty, partial, exact capacity and repeated wrapping in chronological order');

for(const [state,name] of ['recording','post_trigger','fault_frozen','stopped_frozen','budget_stopped'].entries()) {
  const capture=decodeM2AdcCapture(fixture(1,state));
  assert.equal(capture.state,state);
  assert.equal(capture.state_name,name);
}
const noTrigger=fixture(0,0);
noTrigger.writeUInt32LE(0xffffffff,20);
noTrigger.writeUInt32LE(0xffffffff,56);
assert.equal(decodeM2AdcCapture(noTrigger).trigger_at_ms,null);
assert.equal(decodeM2AdcCapture(noTrigger).first_at_ms,null);
const zeroFlags=fixture();
zeroFlags.writeUInt8(0,headerSize+22);
zeroFlags.writeUInt8(0,headerSize+23);
const zeroSample=decodeM2AdcCapture(zeroFlags).samples[0];
for(const name of ['rising','sample_valid','before_seen','reported','accepted','collision'])
  assert.equal(zeroSample[name],false);
assert.equal(zeroSample.blank_before,0);
assert.equal(zeroSample.confirm_after,0);
console.log('PASS M2 ADC units, freeze states, sentinel timestamps, flags and qualification nibbles');

const wrappedClock=fixture(129);
wrappedClock.writeUInt32LE(0xfffffff0,headerSize+sampleSize);
wrappedClock.writeUInt32LE(80,headerSize+sampleSize*2);
const wrappedSamples=decodeM2AdcCapture(wrappedClock).samples;
assert.equal(wrappedSamples[0].at_ms,0xfffffff0/160000);
assert.equal(wrappedSamples[1].at_ms,80/160000);
assert.equal(wrappedSamples[0].window_us,0);
assert.equal(wrappedSamples[1].window_us,0.6);
for(let i=1;i<wrappedSamples.length;i++)
  assert(wrappedSamples[i].window_us>wrappedSamples[i-1].window_us);
assert.equal(decodeM2AdcCapture(wrappedClock).window_span_us,129.1);
const largeCount=fixture(128);
largeCount.writeUInt32LE(0xffffffff,12);
const largeSamples=decodeM2AdcCapture(largeCount).samples;
assert.equal(largeSamples.length,128);
assert.equal(largeSamples[0].at_ms,0.128);
assert.equal(largeSamples[127].at_ms,0.127);
console.log('PASS M2 ADC ring order across DWT timestamp wrap and unsigned cumulative counts');

for(const length of [0,headerSize-1,size-1,size+1]) {
  assert.throws(()=>decodeM2AdcCapture(Buffer.alloc(length)));
}
for(const [offset,value] of [[0,0],[4,2],[16,5],[24,7]]) {
  const invalid=fixture();
  invalid.writeUInt32LE(value,offset);
  assert.throws(()=>decodeM2AdcCapture(invalid));
}
for(const [offset,value] of [[20,0],[20,7],[21,10],[22,0x40]]) {
  const invalid=fixture();
  invalid.writeUInt8(value,headerSize+offset);
  assert.throws(()=>decodeM2AdcCapture(invalid));
}
console.log('PASS M2 ADC ABI, fixed capacity, state, step, decision and reserved flag guards');
