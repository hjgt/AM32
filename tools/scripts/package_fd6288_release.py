#!/usr/bin/env python3
"""Build and package an exact model release from the current committed tree."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--profile',required=True);p.add_argument('--output',required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[2];profile_dir=root/a.profile
profile=json.loads((profile_dir/'profile.json').read_text());out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
if subprocess.check_output(['git','status','--porcelain','--untracked-files=no'],cwd=root,text=True).strip():raise SystemExit('Commit tracked source changes before packaging')
commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
branch=subprocess.check_output(['git','branch','--show-current'],cwd=root,text=True).strip()
if branch!=profile['branch']:raise SystemExit('Wrong model branch: '+branch)
args=json.loads((profile_dir/'build-args.json').read_text());subprocess.run(['make',*args],cwd=root,check=True)
model=profile['model'];prefix=model+'_PURE';obj=root/profile['application_build_dir'];built=profile['application_build_name']
for ext in ['bin','hex','elf']:shutil.copyfile(obj/(built+'.'+ext),out/(profile['application_basename']+'.'+ext))
for component in ['bootloader_basename','settings_basename']:
 for ext in ['bin','hex']:shutil.copyfile(profile_dir/(profile[component]+'.'+ext),out/(profile[component]+'.'+ext))
shutil.copyfile(profile_dir/'build-args.json',out/(prefix+'_BUILD_ARGS.json'))
shutil.copyfile(profile_dir/'README.md',out/(prefix+'_README.md'))
manifest={**profile,'source_repository':'https://github.com/hjgt/AM32','source_commit':commit,'source_branch':branch,'source_tag':profile['tag'],'compiler':subprocess.check_output(['arm-none-eabi-gcc','-dumpfullversion'],text=True).strip(),'diagnostic_arrays':False,'application_end_exclusive':'0x0800f800','bin_bytes':59392,'build_args':prefix+'_BUILD_ARGS.json'}
manifest['files']={x.name:{'sha256':hashlib.sha256(x.read_bytes()).hexdigest(),'bytes':x.stat().st_size} for x in sorted(out.iterdir()) if x.is_file() and x.suffix in ['.bin','.hex','.elf']}
(out/(prefix+'_MANIFEST.json')).write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
tracked=subprocess.check_output(['git','ls-files'],cwd=root,text=True).splitlines()
selected=[x for x in tracked if x.startswith(('Inc/','Src/','Mcu/g431/Src/','tests/','release/'+model.lower()+'-pure/','tools/scripts/build_')) or x in ['Makefile','g431makefile.mk']]
source_hashes={'source_commit':commit,'files':{x:hashlib.sha256((root/x).read_bytes()).hexdigest() for x in selected if (root/x).is_file()}}
(out/(prefix+'_SOURCE_SHA256.json')).write_text(json.dumps(source_hashes,ensure_ascii=False,indent=2)+'\n')
(out/(prefix+'_RELEASE_NOTES.md')).write_text('# '+model+' PURE · 2026-11-20\n\n源码分支：`'+branch+'`；提交：`'+commit+'`。\n\n'+(profile_dir/'README.md').read_text())

# Git archive embeds this exact commit; generated binaries stay outside the source tree.
subprocess.run(['git','archive','--format=tar.gz','--prefix='+model.lower()+'-source/','--output='+str(out/(prefix+'_SOURCES_20261120.tar.gz')),commit],cwd=root,check=True)
files=[x for x in sorted(out.iterdir()) if x.is_file() and not x.name.endswith('_SHA256SUMS.txt')]
(out/(prefix+'_SHA256SUMS.txt')).write_text(''.join(hashlib.sha256(x.read_bytes()).hexdigest()+'  '+x.name+'\n' for x in files))
print(json.dumps({'model':model,'commit':commit,'branch':branch,'output':str(out)},ensure_ascii=False))
