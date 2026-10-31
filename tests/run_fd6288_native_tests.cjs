// Both control policies, with observation compiled in/out. No MCU access.
const fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const {spawnSync}=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-native-tests-'));
for(const native of [false,true]) for(const clean of [false,true]) for(const hold of (native?[false,true]:[false])) for(const launch of (hold?[false,true]:[false])) for(const settle of (launch?[false,true]:[false])) {
 const exe=path.join(dir,`test-${native}-${clean}-${hold}-${launch}-${settle}`);
 for(const [cmd,args] of [[process.env.CC||'cc',[
  '-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
  '-DTARGET_FD_ALIGN_BUDGET',...(native?['-DTARGET_FD_NATIVE_EARLY']:[]),
  ...(hold?['-DTARGET_FD_ALIGN_HOLD']:[]),
  ...(launch?['-DTARGET_FD_LAUNCH_ADVANCE']:[]),
  ...(settle?['-DTARGET_FD_ALIGN_SETTLE']:[]),
  ...(clean?['-DTARGET_FD_NO_DIAGNOSTICS']:[]),
  'tests/fd6288_double_align_tests.c','-o',exe]],[exe,[]]]) {
  const r=spawnSync(cmd,args,{cwd:root,stdio:'inherit'});assert.equal(r.status,0);
 }
}
console.log('PASS old/new early-edge policy with diagnostics enabled/disabled');
