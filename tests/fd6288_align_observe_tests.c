#include <assert.h>
#include <stdio.h>
#include "../Inc/fd6288_align_observe.h"
int main(int argc,char **argv){
 FdAlignObserve s;uint16_t a[]={100,200,300},b[]={90,220,280};
 fdAlignBegin(&s,7);
 fdAlignSample(&s,2,0,a,1);fdAlignSample(&s,2,799999,b,1);
 fdAlignSample(&s,2,1,b,0);
 assert(s.bins[0].total==3&&s.bins[0].valid==2&&s.bins[0].sum[0]==190);
 assert(s.bins[0].minimum[0]==90&&s.bins[0].maximum[1]==220);
 fdAlignSample(&s,2,800000,a,1);assert(s.bins[1].valid==1);
 fdAlignSample(&s,2,4800000,a,1);assert(!s.bins[6].valid);
 fdAlignSample(&s,1,0,b,1);assert(s.bins[6].first[0]==90);
 fdAlignSample(&s,1,9599999,a,1);assert(s.bins[17].valid==1);
 fdAlignSample(&s,1,9600000,a,1);fdAlignSample(&s,0,0,a,1);
 s.frozen=1;fdAlignSample(&s,1,0,a,1);assert(s.bins[6].valid==1);
 if(argc==2){FILE*f=fopen(argv[1],"wb");assert(f);assert(fwrite(&s,sizeof(s),1,f)==1);fclose(f);}
 fdAlignBegin(&s,8);assert(s.trial==8&&!s.frozen&&!s.bins[0].total);
 puts("PASS alignment observer: bins, valid screen, extrema/sums, bounds, freeze, reset");
}
