// Differential execution: same synthetic edge schedules, S4P vs corrected S4R2.
// Not a motor simulation. Proves no fallback interference within current budget.
const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const pre=fs.readFileSync(path.join(__dirname,'fd6288_reposition_tests.c'),'utf8').split('static void align(')[0];
const stalePrefix=process.argv.includes('--stale')?'#define TARGET_FD_STALE_RETRY\n#define TARGET_FD_RETRY_ADJACENT\n':'';
const fullRetry=process.argv.includes('--full-retry');
const body=`
int main(void){
 for(unsigned scenario=0;scenario<8;scenario++)for(unsigned wrap=0;wrap<2;wrap++){
  uint32_t start=wrap?UINT32_MAX-1000u:0u;FdRevPolicy p;fdRevBeginPolicy(&p,start);
  for(uint32_t age=0;age${fullRetry?'<':'<='}FD_REV_ACQUIRE_TICKS;age+=20u*FD_REV_US){
   uint32_t now=start+age;
   if(fdRevDeadlinePolicy(&p,now,start)){printf("%u %u stop %u\\n",scenario,wrap,age);break;}
   uint32_t elapsed=now-p.sector_at;
   unsigned permit=scenario && (scenario<3 || age>185000u*FD_REV_US);
   if(scenario==4)permit=permit && p.confidence==0;
   if(scenario==5)permit=permit && age<240000u*FD_REV_US;
   if(scenario==6)permit=permit && age%1700000u<400000u;
   uint32_t edge_delay=(scenario==2?6000u:scenario==7?17000u:2000u)*FD_REV_US;
   if(permit && p.stage==FD_REV_OPEN && elapsed>=edge_delay)fdRevEdgePolicy(&p,now,8);
   unsigned action=fdRevAdvancePolicy(&p,now);
   if(action)printf("%u %u %u %u %u %u %u %u %u\\n",scenario,wrap,age,action,p.stage,p.confidence,p.period,p.measured,p.delay);
   if(p.stage==FD_REV_CLOSED)break;
  }
 }
 return 0;
}`;
const tmp=fs.mkdtempSync(path.join(os.tmpdir(),'fd-reposition-diff-'));let baseline;
for(const enabled of [false,true]){
 const source=(enabled?stalePrefix+(fullRetry?'#define TARGET_FD_FULL_RETRY\n':'')+pre:pre.replace('#define TARGET_FD_REPOSITION',''))+body,exe=path.join(tmp,'test-'+enabled);
 const cc=spawnSync(process.env.CC||'cc',['-std=c11','-Wall','-Wextra','-Werror','-I'+__dirname,'-x','c','-','-o',exe],{input:source,encoding:'utf8'});
 assert.equal(cc.status,0,cc.stderr);const run=spawnSync(exe,[],{encoding:'utf8'});assert.equal(run.status,0,run.stderr);
 if(enabled)assert.equal(run.stdout,baseline,'Corrected fallback changed baseline schedule');else baseline=run.stdout;
}
console.log('PASS S4P/'+(fullRetry?'S4T before390ms':'S4R2')+' differential schedules: no edges, early/late/intermittent edges, handoff and DWT wrap');
