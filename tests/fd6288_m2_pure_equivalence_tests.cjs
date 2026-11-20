// Diagnostic removal must preserve the helicopter controller's trajectory.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),cp=require('node:child_process'),assert=require('node:assert/strict');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'m2-pure-eq-')),root=path.resolve(__dirname,'..');
const source=`
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "fd6288_m2_heli_start.h"
int main(void){
 FdM2HeliStart s;uint32_t origin=0xffff0000u,last=origin,step=3u,hash=2166136261u;
 fdM2HeliBegin(&s,1u,origin,origin,256000u,step,400u);uint16_t output=400u;
 for(uint32_t us=50u;us<=5000000u;us+=50u){
  uint32_t now=origin+us*160u;
  if((uint32_t)(now-last)>=256000u && !(us>=1500000u&&us<1530000u)){
   step=step==6u?1u:step+1u;fdM2HeliEdge(&s,now,step,1u);last=now;
  }
  uint16_t requested=us>=1000000u&&us<1100000u?700u:2000u;
  if(us>=2000000u&&us<2050000u&&output>500u)output=500u;
  output=fdM2HeliDuty(&s,now,requested,output);
  uint32_t values[]={output,s.healthy,s.reason,s.period,s.turn_edges,s.active};
  for(unsigned i=0;i<6;i++)hash=(hash^values[i])*16777619u;
 }
 assert(output==2000u);printf("%u\\n",hash);return 0;
}
`;
const file=path.join(dir,'test.c');fs.writeFileSync(file,source);const outputs=[];
for(const pure of [false,true]){
 const exe=path.join(dir,pure?'pure':'diagnostic');
 for(const [cmd,args] of [['cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),...(pure?['-DTARGET_FD_M2_PURE']:[]),file,'-o',exe]],[exe,[]]]){
  const r=cp.spawnSync(cmd,args,{encoding:'utf8'});if(r.stderr)process.stderr.write(r.stderr);assert.equal(r.status,0);if(cmd===exe)outputs.push(r.stdout);
 }
}
assert.equal(outputs[0],outputs[1]);
console.log('PASS pure/diagnostic heli outputs and functional states match at100000ticks across throttle steps, feedback loss/recovery, caps and DWTwrap');
