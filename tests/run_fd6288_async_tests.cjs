const fs=require('node:fs'),path=require('node:path'),os=require('node:os');
const assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'), main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
const begin=main.indexOf('volatile FdAsyncDiagnostics fd_async_diag');
const end=main.indexOf('\n#endif\n\n#ifdef TARGET_FD_ADC_JEOS',begin);
assert(begin>0 && end>begin);
const irqBegin=main.indexOf('void fdAdcJeosService(void)\n{',end);
const irqEnd=main.indexOf('\n#endif\n\nvoid commutate()',irqBegin);
assert(irqBegin>end && irqEnd>irqBegin);
const source=fs.readFileSync(path.join(__dirname,'fd6288_async_tests.c'),'utf8')
 .replace('/* IMPLEMENTATION */',main.slice(begin,end)+'\n'+main.slice(irqBegin,irqEnd));
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-async-')),exe=path.join(dir,'test');
for(const [cmd,args,input] of [
 [process.env.CC||'cc',[...(process.argv.includes('--reposition')?['-DTARGET_FD_REPOSITION']:[]),...(process.argv.includes('--settle')?['-DTARGET_FD_ALIGN_SETTLE']:[]),...(process.argv.includes('--align')?['-DTARGET_FD_DOUBLE_ALIGN']:[]),...(process.argv.includes('--clean')?['-DTARGET_FD_NO_DIAGNOSTICS']:[]),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),'-x','c','-','-o',exe],source],
 [exe,[],undefined]]) {
 const r=spawnSync(cmd,args,{input,encoding:'utf8'});
 process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}
assert(main.includes('#ifndef TARGET_FD_ADC_JEOS\n        if (old_routine && running)'));
assert(main.includes('!defined(TARGET_FD_ASYNC_COMM)'));
console.log('PASS actual scheduler: deadline/early/late/wrap, duplicate, cancellation, zero, fault, epoch, direction/coast, IRQ mask');
