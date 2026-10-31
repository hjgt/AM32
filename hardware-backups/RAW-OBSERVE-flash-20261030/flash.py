import socket,json,time,pathlib,hashlib,subprocess
root=pathlib.Path.cwd();out=root/'hardware-backups/RAW-OBSERVE-flash-20261030';log=[]
hexfile=root/'obj-raw-observe-20261030/AM32_CUSTOM_FD6288_G431_2.20.hex'
assert hashlib.sha256(hexfile.read_bytes()).hexdigest()=='2afdf1ecb383d13471be695ca78fb125261c1532b4c5248bd0e30dd1d74f4eb7'
base=0
for line in hexfile.read_text().splitlines():
 b=bytes.fromhex(line[1:]);assert sum(b)%256==0
 if b[3]==4:base=int.from_bytes(b[4:6],'big')<<16
 if b[3]==0:assert 0x08001000<=base+int.from_bytes(b[1:3],'big') and base+int.from_bytes(b[1:3],'big')+b[0]<=0x0800f800
s=socket.create_connection(('127.0.0.1',6666),5);s.settimeout(30)
def rpc(cmd):
 s.sendall(('if {[catch {'+cmd+'} r]} {format "ERR:%s" $r} else {format "OK:%s" $r}\x1a').encode());v=b''
 while b'\x1a' not in v:v+=s.recv(65536)
 t=v.split(b'\x1a')[0].decode();assert t.startswith('OK:'),t
 return t[3:]
def save(stage,value):
 log.append(dict(stage=stage,value=value));(out/'flash.json').write_text(json.dumps(log,indent=2));print(stage,value,flush=True)
def mem(a,bits=32,n=1):return [int(v,0) for v in rpc(f'read_memory {a:#x} {bits} {n}').split()]
try:
 rpc('adapter speed 100')
 assert mem(0x48000010)[0]&0x1f00==0
 assert mem(0x48001410)[0]&1==0
 assert mem(0x40012c34,32,4)==[0,0,0,0]
 save('stopped_bridge_before',True)
 settings=mem(0x0800f800,8,256)
 save('reset_init',rpc('reset init'))
 save('write',rpc('flash write_image erase {'+str(hexfile)+'}'))
 save('verify',rpc('verify_image {'+str(hexfile)+'}'))
 assert mem(0x0800f800,8,256)==settings
 save('settings_unchanged',True)
 save('reset_run',rpc('reset run'));time.sleep(3)
 state={n:mem(a,8,z)[0] if z==1 else sum(x<<(8*i) for i,x in enumerate(mem(a,8,z))) for n,a,z in [('armed',0x20004675,1),('input',0x20004672,2),('adjusted_input',0x20004668,2),('running',0x200042c4,1),('duty_cycle',0x200042c8,2)]}
 save('stopped_after_reset',state)
 assert all(state[n]==0 for n in ['input','adjusted_input','running','duty_cycle'])
 assert mem(0x40012c34,32,4)==[0,0,0,0]
 save('zero_outputs',True)
except Exception as e:
 save('error',str(e));raise
finally:s.close()
