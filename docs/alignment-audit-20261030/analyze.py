import pathlib,json,csv
root=pathlib.Path(__file__).resolve().parents[2];rows=[]
for p in sorted((root/'hardware-backups').glob('*/capture*.json')):
 try:x=json.loads(p.read_text())
 except (ValueError,OSError):continue
 if not isinstance(x,dict) or not x.get('alignment'):continue
 for b in x['alignment'].get('bins',[]):
  if b.get('vector')!=2 or not b.get('valid'):continue
  a,f,l=[z['mean'] for z in b['phases']]
  rows.append(dict(file=str(p.relative_to(root)),trial=x['header']['trial'],reason=x['header']['finish_reason'],bin_ms=b['start_ms'],n=b['valid'],a=a,b=f,c=l,mid_error=f-(a+l)/2,normalized=(f-(a+l)/2)/(a-l)))
out=root/'docs/alignment-audit-20261030/alignment.csv'
with out.open('w') as f:
 w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
print(f'{len(rows)} bins, {len(set(r["file"] for r in rows))} captures')
