const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function section(start,end){const a=main.indexOf(start),b=main.indexOf(end,a);assert(a>0&&b>a);return main.slice(a,b);}
const source=fs.readFileSync(path.join(__dirname,'fd6288_reposition_adapter_tests.c'),'utf8').replace('/* ACTUAL_ADAPTER */',
 section('static void fdRevTick(void)\n{','\nstatic void fdRevObserveEdge(void)')+'\n'+
 section('static void fdBenchWritePwm(uint16_t ticks)\n{','\n/* FD_POWER_BENCH_ADAPTER_END */'));
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-reposition-')),exe=path.join(dir,'test');
for(const [cmd,args,input] of [[process.env.CC||'cc',[...(process.argv.includes('--stale')?['-DTARGET_FD_STALE_RETRY']:[]),...(process.argv.includes('--adjacent')?['-DTARGET_FD_RETRY_ADJACENT']:[]),...(process.argv.includes('--full-retry')?['-DTARGET_FD_FULL_RETRY']:[]),'-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+__dirname,'-x','c','-','-o',exe],source],[exe,[],undefined]]){
 const r=spawnSync(cmd,args,{cwd:root,input,encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}
