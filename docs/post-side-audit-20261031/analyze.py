import json
from pathlib import Path
root = Path(__file__).resolve().parents[2]
files = ['RAW-OBSERVE-flash-20261030/capture-trial8.bin.json', 'FIRST500-flash-20261030/capture-last.bin.json', 'ALIGN-PAIR-flash-20261030/capture-20261031-trial10.bin.json', 'ALIGN-PAIR-flash-20261030/capture-20261031-success.bin.json']
out=[]
for name in files:
    d=json.loads((root/'hardware-backups'/name).read_text())
    sector=next(x for x in d['records'] if x['samples'])
    raw=[x for x in d['edge_observer']['records'] if x.get('window')=='first_drive']
    selected=raw[:3]+raw[-1:] if raw else []
    out.append({'file':name,'trial':d['header']['trial'],'first_drive_sector':sector,'raw_boundary_and_tail':[{**x,'age_from_sector_us':x['at_us']-sector['start_us'],'recomputed_directed':(x['a']+x['b']+x['c'])//3-x['a']} for x in selected]})
print(json.dumps(out,indent=2))
