import socket,json,pathlib
out=pathlib.Path('hardware-backups/LAUNCH-HOLD-flash-20261106')
image=pathlib.Path('test-firmware/fd6288-LAUNCH-HOLD-20261106/AM32_CUSTOM_FD6288_G431_2.20.hex').resolve()
log=[{'operator':'User agreed to isolated launch vector30ms after30+300ms alignment; direct flash and skipped readback follow session authorization; live zero gate below','image':str(image)}]
s=socket.create_connection(('127.0.0.1',6666),5);s.settimeout(30)
def rpc(cmd):
 s.sendall(('if {[catch {'+cmd+'} r]} {format "ERR:%s" $r} else {format "OK:%s" $r}\x1a').encode());v=b''
 while b'\x1a' not in v:
  data=s.recv(65536)
  if not data:raise RuntimeError('OpenOCD disconnected')
  v+=data
 t=v.split(b'\x1a')[0].decode()
 log.append({'command':cmd,'result':t});(out/'flash.json').write_text(json.dumps(log,indent=2));print(t,flush=True)
 if not t.startswith('OK:'):raise RuntimeError(t)
 return t
try:
 import re,subprocess
 syms={}
 elf='test-firmware/fd6288-ALIGN-TAIL-20261106/AM32_CUSTOM_FD6288_G431_2.20.elf'
 for line in subprocess.check_output(['/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin/arm-none-eabi-nm','-S','--defined-only',elf],text=True).splitlines():
  m=re.match(r'^([0-9a-fA-F]+) ([0-9a-fA-F]+) \w (\w+)$',line)
  if m:syms[m[3]]=(int(m[1],16),int(m[2],16))
 for name in ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active']:
  addr,size=syms[name]
  value=rpc('read_memory 0x%x 8 %d'%(addr,size))
  assert all(int(x,0)==0 for x in value[3:].split()),name+' is not zero; no flash attempted'
 for addr in [0x40012c34,0x40012c38,0x40012c3c,0x40012c40]:
  value=rpc('read_memory 0x%x 32 1'%addr)
  assert int(value[3:].strip(),0)==0,'PWM is not zero; no flash attempted'
 rpc('reset init')
 rpc('flash write_image erase {'+str(image)+'}')
 rpc('reset run')
finally:s.close()
