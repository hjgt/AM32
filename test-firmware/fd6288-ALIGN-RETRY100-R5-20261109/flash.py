"""After explicit flash instruction: stopped-board gate, backup settings, write
main only, retaining the current original parameter page, reset run. No firmware readback or throttle.
Requires an already running local OpenOCD. Default is no action; pass --execute.
"""
import argparse, hashlib, json, pathlib, re, socket, subprocess, time
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--execute',action='store_true')
parser.add_argument('--out',default='hardware-backups/ALIGN-RETRY100-R5-flash-20261109/attempt1')
args=parser.parse_args()
assert args.execute,'No action taken: --execute required after user flash instruction'
package=pathlib.Path(__file__).resolve().parent
root=package.parent.parent
out=root/args.out
out.mkdir(parents=True,exist_ok=True)
logfile=out/'flash.json'
assert not logfile.exists(),'Use a fresh output directory; preserve earlier attempts'
image=package/'AM32_CUSTOM_FD6288_G431_2.20.hex'
oldelf=root/'test-firmware/fd6288-ALIGN-RETRY-20261109/AM32_CUSTOM_FD6288_G431_2.20.elf'
newelf=image.with_suffix('.elf')
hashes=json.loads((package/'SHA256.json').read_text())
for file in [image,package/'settings-expected.bin',newelf]:
    assert hashlib.sha256(file.read_bytes()).hexdigest()==hashes[file.name],file.name
log=[{'image':str(image),'settings_written':False,'behavior':'Full ALIGN-PAIR startup/run, no100ms motor cutoff; startup-only6 attempts total (first+5 retries),30+100ms alignment,100ms off,3.08s acquisition cap','preflash_elf':str(oldelf),'postflash_elf':str(newelf),'readback_verification':'skipped as requested','throttle_sent':False}]
def save():logfile.write_text(json.dumps(log,indent=2)+'\n')
save()
s=socket.create_connection(('127.0.0.1',6666),5)
s.settimeout(30)
def rpc(cmd):
    s.sendall(('if {[catch {'+cmd+'} r]} {format "ERR:%s" $r} else {format "OK:%s" $r}\x1a').encode())
    b=b''
    while b'\x1a' not in b:
        part=s.recv(65536)
        if not part:raise RuntimeError('OpenOCD disconnected')
        b+=part
    value=b.split(b'\x1a')[0].decode()
    log.append({'command':cmd,'result':value});save()
    assert value.startswith('OK:'),value
    return value[3:]
def mem(a,n):
    if n>512:return b''.join(mem(a+i,min(512,n-i)) for i in range(0,n,512))
    values=[int(x,0) for x in rpc('read_memory 0x%x 8 %d'%(a,n)).split()]
    assert len(values)==n and all(0<=x<=255 for x in values)
    return bytes(values)
def words(a,n=1):
    values=[int(x,0) for x in rpc('read_memory 0x%x 32 %d'%(a,n)).split()]
    assert len(values)==n
    return values
def symbols(elf):
    result={}
    for line in subprocess.check_output(['/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-nm','-S','--defined-only',str(elf)],text=True).splitlines():
        m=re.match(r'^([0-9a-f]+) ([0-9a-f]+) \w (.+)$',line)
        if m:result[m[3]]=(int(m[1],16),int(m[2],16))
    return result
def stopped(syms):
    assert rpc('[target current] curstate').strip()=='running'
    assert words(0xe000ed08)[0]==0x08001000
    for name in ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active']:
        a,n=syms[name]
        assert int.from_bytes(mem(a,n),'little')==0,'Not stopped: '+name
    assert words(0x48000010)[0]&0x1f00==0
    assert words(0x48001410)[0]&1==0
    assert all(x==0 for x in words(0x40012c34,4))
try:
    stopped(symbols(oldelf))
    before=mem(0x0800f800,2048)
    (out/'settings-before.bin').write_bytes(before)
    assert before==(package/'settings-expected.bin').read_bytes(),'Current settings changed: retain backup and review before flash'
    log.append({'settings_before_sha256':hashlib.sha256(before).hexdigest(),'settings_written':False});save()
    stopped(symbols(oldelf))
    rpc('reset init')
    print(rpc('flash write_image erase {'+str(image)+'}'),flush=True)
    print(rpc('reset run'),flush=True)
    log.append({'main_written':True,'settings_written':False});save()
    time.sleep(2.5)
    syms=symbols(newelf)
    stopped(syms)
    a,n=syms['eepromBuffer'];ram=mem(a,min(n,48))
    original=(package/'settings-expected.bin').read_bytes()
    assert ram[:48]==original[:48],'Runtime parameters differ from original backup'
    first=words(0xe0001000,2);demcr=words(0xe000edfc)[0];second=words(0xe0001000,2)
    clock={'ctrl':second[0],'demcr':demcr,'delta':(second[1]-first[1])&0xffffffff}
    assert clock['ctrl']&1 and demcr&0x1000000 and clock['delta']>0
    log.append({'completed':True,'runtime_clock':clock,'runtime_sine':ram[19],'runtime_pwm_khz':ram[24],'throttle_sent':False});save()
    print('FLASH COMPLETE: ALIGN_RETRY100_R5; original settings retained, outputs off, clock advancing.',flush=True)
finally:s.close()
