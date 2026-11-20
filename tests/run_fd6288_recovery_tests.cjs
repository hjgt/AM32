const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-recovery-')),exe=path.join(dir,'test');
for(const [cmd,args]of [[process.env.CC||'cc',['-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',path.join(__dirname,'fd6288_recovery_policy_tests.c'),'-o',exe]],[exe,[]]]){
 const r=spawnSync(cmd,args,{stdio:'inherit'});assert.equal(r.status,0);
}
