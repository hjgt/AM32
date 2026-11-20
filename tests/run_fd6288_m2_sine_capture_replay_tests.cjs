const fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'m2-capture-lock-'));
for(const kind of ['success','failed']){
 const name=`capture-m2-sine-500-tracked-handoff-hold200-${kind}-20261118.json`;
 const d=JSON.parse(fs.readFileSync(path.join(root,'docs/m2-startup',name),'utf8'));
 const sectors=d.handoff.sectors,origin=sectors[0].at_ms;
 const rows=sectors.map((s,i)=>`{${Math.round((s.at_ms-origin)*160000)}u,${Math.round((s.edge_ms-origin)*160000)}u,${Math.round((d.adaptation.records[i].due_us/1000-origin)*160000)}u,${s.step-1}u}`);
 const period=Math.round(d.handoff.coast_period_us*160);
 const source=`#include <assert.h>
#include <stdio.h>
#include "fd6288_m2_sine_adaptive.h"
static const struct {uint32_t entry,edge,due,sector;} r[]={${rows.join(',')}};
int main(void){
 FdM2SineLock s={.period=${period}u,.reference_period=${period}u,.forward=1};
 unsigned ready=0;
 for(unsigned i=0;i<sizeof(r)/sizeof(r[0]);i++){
  s.sector=r[i].sector;s.sector_at=r[i].entry;s.due=r[i].entry+s.period;s.observed=0;
  fdM2SineLockObserve(&s,r[i].edge);
  if(fdM2SineLockReady(&s,r[i].due)){ready=i+1;break;}
 }
 assert(ready==9u);puts("PASS ${kind} captured event train: full-turn and consecutive lock at event nine");
}`;
 const file=path.join(dir,kind+'.c'),exe=path.join(dir,kind);fs.writeFileSync(file,source);
 for(const [cmd,args] of [['cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),file,'-o',exe]],[exe,[]]]){
  const r=cp.spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
 }
}
