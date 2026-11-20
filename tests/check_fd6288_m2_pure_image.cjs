const fs=require('node:fs'),cp=require('node:child_process'),assert=require('node:assert/strict');
const [elf,bin]=process.argv.slice(2);assert(elf&&bin,'ELF BIN required');
const symbols=new Map();
for(const row of cp.execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}).split('\n')){
 const m=row.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);if(m)symbols.set(m[3],parseInt(m[2],16));
}
for(const n of ['fd_m2_wire','fd_m2_sine_adc_capture','fd_m2_sine_powered','fd_m2_sine_closed','fd_m2_sine_adaptation','fd_sine_sensors','fd_sine_pins','fd_sine_pwm_snapshot','fdM2WireRecord','fdSineRecordSensors'])assert(!symbols.has(n),'Observer present: '+n);
for(const [n,size] of [['fd_m2_heli',44],['fd_power_bench',12],['fd_async_diag',24],['fd_native_sine',12],['fd_m2_sine_coast',1],['fd_m2_sine_handoff',4],['fd_m2_sine_recovery',8]])assert.equal(symbols.get(n),size,n);
for(const n of ['commutate','getCompOutputSample','zcfoundroutine','fdAdcJeosService','loadEEpromSettings'])assert(symbols.has(n),'Control missing: '+n);
const bytes=fs.readFileSync(bin);assert(bytes.subarray(-32).includes(Buffer.from('M2_PURE_FD6288_G431')));
console.log('PASS M2 PURE identity, no observation buffers/functions, retained minimal controller/scheduler state');
