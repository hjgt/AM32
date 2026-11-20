const fs=require('node:fs'),cp=require('node:child_process'),assert=require('node:assert/strict');
const [elf,bin]=process.argv.slice(2);assert(elf&&bin,'ELF BIN required');
const symbols=new Map();
for(const row of cp.execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}).split('\n')){
 const m=row.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);if(m)symbols.set(m[3],parseInt(m[2],16));
}
for(const n of ['fd_light_log','fd_edge_observe','fd_sector_diag','fd_align_observe','fd_start_retry_diag','fd_reposition_diag','fd_startup_wave','adc_zcd_diag','fd_scope_launch'])assert(!symbols.has(n),'Observer remains: '+n);
for(const [n,size] of [['fd_m1_pair_control',8],['fd_async_diag',24],['fd_power_bench',16],['fd_rev',64]])assert.equal(symbols.get(n),size,n);
for(const n of ['commutate','getCompOutputSample','zcfoundroutine','fdAdcJeosService','loadEEpromSettings','fdTraceInit'])assert(symbols.has(n),'Control missing: '+n);
assert(fs.readFileSync(bin).subarray(-32).includes(Buffer.from('M1_PURE_FD6288_G431')));
console.log('PASS M1 PURE image: observers absent; startup, ADC, scheduler, DWT and minimal pair timing state retained.');
