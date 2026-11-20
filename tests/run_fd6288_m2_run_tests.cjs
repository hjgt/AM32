const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),dir=fs.mkdtempSync(path.join(os.tmpdir(),'m2-run-')),exe=path.join(dir,'test');
for(const [cmd,args] of [['cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),path.join(__dirname,'fd6288_m2_run_tests.c'),'-o',exe]],[exe,[]]]){
 const r=cp.spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}
