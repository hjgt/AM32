import socket,json,pathlib
out=pathlib.Path('hardware-backups/ALIGN-ONLY-LONG-flash-20261105')
image=pathlib.Path('test-firmware/fd6288-ALIGN-ONLY-LONG-20261105/AM32_CUSTOM_FD6288_G431_2.20.hex').resolve()
log=[{'operator':'User requested implementation of long observation version; direct flashing and skipped readback follow prior authorization','image':str(image)}]
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
try:
 rpc('reset init')
 rpc('flash write_image erase {'+str(image)+'}')
 rpc('reset run')
finally:s.close()
