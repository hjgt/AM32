import socket, json, pathlib, re, subprocess, hashlib
root=pathlib.Path('/Users/nxf75465/codex/am32_on_omp_esc/AM32-main')
out=root/'hardware-backups/NATIVE-SINE-flash-20261108'
out.mkdir(exist_ok=True)
logfile=out/'flash.json'
assert not logfile.exists(), 'Do not overwrite a prior flash record'
image=root/'test-firmware/fd6288-NATIVE-SINE-INHERIT-TIMING-20261108/NATIVE-SINE-COMBINED.hex'
assert hashlib.sha256(image.read_bytes()).hexdigest()=='ec4c8c583a5df747608f938dd4801acb439364efe8ea62b01c037215a1bdf104'
elf=root/'test-firmware/fd6288-EARLY-REKICK-20261107/AM32_CUSTOM_FD6288_G431_2.20.elf'
log=[{'authorization':'User explicitly requested flash on 2026-11-08; combined main/settings, preserve timing, skip post-write readback verification; never send throttle','image':str(image),'preflash_elf':str(elf),'previous_settings_backup':'hardware-backups/NATIVE-SINE-parameters-before-20261108/'}]
def save():logfile.write_text(json.dumps(log,indent=2)+'\n')
save()
s=socket.create_connection(('127.0.0.1',6666),5);s.settimeout(30)
def rpc(cmd):
 s.sendall(('if {[catch {'+cmd+'} r]} {format "ERR:%s" $r} else {format "OK:%s" $r}\x1a').encode());b=b''
 while b'\x1a' not in b:
  x=s.recv(65536)
  if not x:raise RuntimeError('OpenOCD disconnected')
  b+=x
 value=b.split(b'\x1a')[0].decode();log.append({'command':cmd,'result':value});save()
 if not value.startswith('OK:'):raise RuntimeError(value)
 return value[3:]
def mem(a,n):
 v=[int(x,0) for x in rpc('read_memory 0x%x 8 %d'%(a,n)).split()]
 assert len(v)==n and all(0<=x<=255 for x in v)
 return bytes(v)
try:
 syms={}
 nm=subprocess.check_output(['/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-nm','-S','--defined-only',str(elf)],text=True)
 for row in nm.splitlines():
  m=re.match(r'^([0-9a-f]+) ([0-9a-f]+) \w (.+)$',row)
  if m:syms[m[3]]=(int(m[1],16),int(m[2],16))
 assert rpc('[target current] curstate').strip()=='running'
 assert int.from_bytes(mem(0xe000ed08,4),'little')==0x08001000
 for name in ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active']:
  a,n=syms[name];assert int.from_bytes(mem(a,n),'little')==0,'Not stopped: '+name
 assert int.from_bytes(mem(0x48000010,4),'little')&0x1f00==0,'PA gates not off'
 assert int.from_bytes(mem(0x48001410,4),'little')&1==0,'PF gate not off'
 for i in range(4):assert int.from_bytes(mem(0x40012c34+4*i,4),'little')==0,'PWM not zero'
 # Keep the just-backed-up timing; never silently overwrite a newer setting.
 old=(root/'hardware-backups/NATIVE-SINE-parameters-before-20261108/settings-page.bin').read_bytes()
 current=mem(0x0800f800,48)
 assert current==old[:48],'Settings changed since backup; prepare a new matching configuration first'
 print('Throttle/output zero; writing native sine main + configuration.',flush=True)
 print(rpc('reset init'),flush=True)
 print(rpc('flash write_image erase {'+str(image)+'}'),flush=True)
 print(rpc('reset run'),flush=True)
 log.append({'completed':True,'readback_verification':'skipped as requested','throttle_sent':False});save()
 print('FLASH COMPLETE; reset run succeeded; no readback verification.',flush=True)
finally:s.close()
