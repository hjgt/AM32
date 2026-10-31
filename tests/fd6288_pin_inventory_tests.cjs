const assert=require('node:assert/strict'),fs=require('node:fs'),os=require('node:os'),path=require('node:path');
const {decode,capture}=require('../tools/scripts/fd6288_pin_inventory.cjs');
(async()=>{
 const analog={MODER:0xffffffff,PUPDR:0,IDR:0xffff,AFRL:0,AFRH:0};
 assert(decode('A',analog).every(p=>p.digital_level===null));
 assert(!decode('A',analog).some(p=>['PA13','PA14'].includes(p.pin)));
 assert(decode('B',{...analog,MODER:0}).every(p=>p.digital_level===1));
 const out=path.join(fs.mkdtempSync(path.join(os.tmpdir(),'fd-pin-test-')),'snapshot.json');
 let closed=false;
 const connection=async()=>({close(){closed=true},async rpc(command){
  assert.match(command,/^read_memory 0x[0-9a-f]+ 32 \d+$/);
  const [,addr,,count]=command.split(' '),a=Number(addr),n=Number(count);
  if(a===0x4002104c)return '35';if(a===0xe000ed08)return String(0x08001000);
  return Array(n).fill(0).join(' ');
 }});
 await assert.rejects(capture(out,false,connection));
 const result=await capture(out,true,connection);
 assert(closed);assert.equal(result.pins.length,9);assert(fs.existsSync(out));
 await assert.rejects(capture(out,true,connection));
 console.log('PASS read-only commands, stop confirmation, exclusive output, excluded SWD pins, analog IDR caveat');
})().catch(e=>{console.error(e);process.exitCode=1});
