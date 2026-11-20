#include <assert.h>
#include <stdio.h>
#define TARGET_FD_SECTOR_DIAG
#include "../Inc/fd6288_sector_diag.h"
volatile FdSectorDiagnostics fd_sector_diag;
int main(int argc,char **argv){
    for(unsigned wrap=0;wrap<2;wrap++){
        uint32_t start=wrap?UINT32_MAX-100:100;
        fdSectorBegin(3,start);
        fdSectorStep(start+5,5,1,1,1);
        fdSectorClose(start+4800000,1);
        assert(fd_sector_diag.records[0].samples==0);
        fdSectorStep(start+4800001,1,2,1,1);
        fdSectorSample(1000,1,0,0,0,0); /* blank excluded from extrema */
        fdSectorSample(-20,2,1,1,0,0);
        fdSectorSample(0,3,1,1,0,0);
        fdSectorSample(15,6,1,1,1,0);
        fdSectorSample(16,7,1,1,2,start+4800100);
        volatile FdSectorRecord *r=&fd_sector_diag.records[1];
        assert(r->samples==5&&r->usable==4&&r->minimum==-20&&r->maximum==16);
        assert(r->first==-20&&r->last==16&&r->pre_seen&&r->confirmations==2);
        assert(r->first_accept==4800100&&r->last_accept==4800100);
        unsigned total=0;for(unsigned i=0;i<10;i++)total+=r->reasons[i];
        assert(total==r->samples);
        fdSectorClose(start+4800200,2);
        fdSectorFinish(start+4800201,0);
        fdSectorSample(20,7,1,1,2,start+4800202);
        fdSectorStep(start+4800300,2,2,0,1);
        assert(fd_sector_diag.sectors==2&&r->samples==5&&r->exit_kind==2);
        fdSectorFinish(start+4800400,8);assert(fd_sector_diag.finish_reason==0);
        fdSectorBegin(4,start);
        for(unsigned n=0;n<40;n++){
            fdSectorStep(start+n*100,1,2,1,1);
            fdSectorSample(-10,2,1,1,0,0);
            fdSectorClose(start+n*100+50,3);
        }
        assert(fd_sector_diag.sectors==40&&fd_sector_diag.records[23].samples==1);
        fdSectorFinish(start+5000,8);
        assert(fd_sector_diag.frozen&&fd_sector_diag.finish_reason==8);
        fdSectorBegin(5,start);fdSectorStep(start,2,2,0,0);
        assert(fd_sector_diag.records[0].samples==0&&fd_sector_diag.records[0].first_accept==UINT32_MAX);
    }
    puts("PASS sector statistics: wrap, blank exclusion, counters, edges, freeze, capacity, restart");
    fdSectorBegin(6,100);
    fdSectorStep(110,1,2,1,1);
    fdSectorSample(-15,2,1,1,0,0);
    fdSectorSample(20,7,1,1,2,200);
    fdSectorFinish(300,8);
    if(argc==2){FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite((const void*)&fd_sector_diag,sizeof(fd_sector_diag),1,f)==1);assert(!fclose(f));}
}
