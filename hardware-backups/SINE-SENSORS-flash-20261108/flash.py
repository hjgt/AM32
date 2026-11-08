import socket,json,pathlib,re,subprocess,hashlib,time
root=pathlib.Path('/Users/nxf75465/codex/am32_on_omp_esc/AM32-main')
out=root/'hardware-backups/SINE-SENSORS-flash-20261108'
out.mkdir(exist_ok=True)
logfile=out/'flash.json'
assert not logfile.exists(),'Do not overwrite a flash record'
image=root/'test-firmware/fd6288-SINE-SENSORS-20261108/AM32_CUSTOM_FD6288_G431_2.20.hex'
assert hashlib.sha256(image.read_bytes()).hexdigest()=='8039cc2cf9a10b315a8e528ef5f80c4d39dfed09d5785289992abd70b7380e39'
oldelf=root/'test-firmware/fd6288-SINE-CENTERED-20261108/AM32_CUSTOM_FD6288_G431_2.20.elf'
newelf=image.with_suffix('.elf')
log=[{'authorization':'User explicitly requested compiling and flashing the prepared SINE-SENSORS telemetry update fix; sine PWM strategy/power and three-second cap retained; preserve already-installed settings, skip firmware readback verification; confirm running control clock before motor test','image':str(image),'preflash_elf':str(oldelf),'postflash_elf':str(newelf),'settings_written':False,'throttle_sent':False}]
def save():logfile.write_text(json.dumps(log,indent=2)+'\n')
save()
s=socket.create_connection(('127.0.0.1',6666),5);s.settimeout(30)
def rpc(cmd):
 s.sendall(('if {[catch {'+cmd+'} r]} {format "ERR:%s" $r} else {format "OK:%s" $r}\x1a').encode());b=b''
 while b'\x1a' not in b:
  chunk=s.recv(65536)
  if not chunk:raise RuntimeError('OpenOCD disconnected')
  b+=chunk
 value=b.split(b'\x1a')[0].decode();log.append({'command':cmd,'result':value});save()
 assert value.startswith('OK:'),value
 return value[3:]
def mem(a,n):
 v=[int(x,0) for x in rpc('read_memory 0x%x 8 %d'%(a,n)).split()];assert len(v)==n and all(0<=x<=255 for x in v);return bytes(v)
def words(a,n=1):
 v=[int(x,0) for x in rpc('read_memory 0x%x 32 %d'%(a,n)).split()];assert len(v)==n;return v
def symbols(elf):
 d={}
 for row in subprocess.check_output(['/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-nm','-S','--defined-only',str(elf)],text=True).splitlines():
  m=re.match(r'^([0-9a-f]+) ([0-9a-f]+) \w (.+)$',row)
  if m:d[m[3]]=(int(m[1],16),int(m[2],16))
 return d
def stopped(syms):
 assert rpc('[target current] curstate').strip()=='running'
 assert words(0xe000ed08)[0]==0x08001000
 for name in ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active']:
  a,n=syms[name];assert int.from_bytes(mem(a,n),'little')==0,'Not stopped: '+name
 assert words(0x48000010)[0]&0x1f00==0,'PA gates active'
 assert words(0x48001410)[0]&1==0,'PF gate active'
 assert all(v==0 for v in words(0x40012c34,4)),'PWM active'
try:
 stopped(symbols(oldelf))
 settings=mem(0x0800f800,48)
 expected=(root/'test-firmware/fd6288-NATIVE-SINE-INHERIT-TIMING-20261108/NATIVE-SINE-SETTINGS.bin').read_bytes()
 assert settings==expected[:48],'Parameters changed since the previous native-sine trial'
 print('Throttle/output zero; flashing main only, keeping installed settings.',flush=True)
 print(rpc('reset init'),flush=True)
 print(rpc('flash write_image erase {'+str(image)+'}'),flush=True)
 print(rpc('reset run'),flush=True)
 log.append({'flash_write_complete':True,'readback_verification':'skipped as requested'});save()
 time.sleep(2.5)
 syms=symbols(newelf)
 stopped(syms)
 a=words(0xe0001000,2);demcr=words(0xe000edfc)[0];b=words(0xe0001000,2)
 addr,n=syms['fd_native_clock_ok'];selftest=int.from_bytes(mem(addr,n),'little')
 state={'dwt_ctrl':b[0],'demcr':demcr,'cyccnt_first':a[1],'cyccnt_second':b[1],'delta':(b[1]-a[1])&0xffffffff,'selftest_ok':selftest}
 log.append({'runtime_control_clock':state});save()
 assert b[0]&1 and demcr&0x1000000 and state['delta']>0 and selftest==1,'Control clock check failed; do not test motor'
 log.append({'completed':True,'runtime_clock_valid':True,'throttle_sent':False});save()
 print('FLASH COMPLETE; DWT enabled, progressing and startup self-test passed.',flush=True)
 print(json.dumps(state),flush=True)
finally:s.close()
