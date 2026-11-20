const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function fn(signature){
 const start=main.indexOf(signature),open=main.indexOf('{',start);assert(start>=0&&open>start);
 if(signature==='static void fdBenchService(void)')return main.slice(start,main.indexOf('static uint8_t fdBenchGuardInput(void)',start));
 let depth=0;for(let i=open;i<main.length;i++){if(main[i]==='{')depth++;if(main[i]==='}'&&!--depth)return main.slice(start,i+1);}throw Error(signature);
}
const ph=fs.readFileSync(path.join(root,'Inc/fd6288_startup_power.h'),'utf8');
const reasons=ph.slice(ph.indexOf('enum {'),ph.indexOf('};',ph.indexOf('enum {'))+2);
const source=fs.readFileSync(path.join(__dirname,'fd6288_blue_start_adapter_tests.c'),'utf8').replace('/* ACTUAL_REASONS */',reasons)
 .replace('/* ACTUAL_SERVICE */',fn('static void fdBenchService(void)'))
 .replace('/* ACTUAL_ADAPTER */',[
  'static void fdBenchStop(uint8_t reason)','static uint8_t fdBenchGuardInput(void)',
  'static void fdRevTick(void)','static void fdRevObserveEdge(void)','static void fdBenchWritePwm(uint16_t ticks)'
 ].map(fn).join('\n'));
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-launch-hold-')),exe=path.join(dir,'test');
for(const [cmd,args,input] of [[process.env.CC||'cc',[...(process.argv.includes('--two')?['-DTARGET_FD_LAUNCH_TWO']:[]),'-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),'-x','c','-','-o',exe],source],[exe,[],undefined]]){
 const r=spawnSync(cmd,args,{input,encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}
