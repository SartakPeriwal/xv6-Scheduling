#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "pinfo.h"

int main (int argc,char *argv[])
{
    int arg1;
    int arg2;
    int status;
    struct proc_stat *pinfo=0;
    pinfo=malloc(sizeof(struct proc_stat));
    int pid[3];
    for(int i=0;i<3;i++)
    {

        pid[i]= fork();
        if (!pid[i] )
        {   
            //exec(argv[1], argv);
            //printf(1, "exec %s failed\n", argv[1]);
            //printf(1, "error process id =0\n");
            for(int i=0;i<100000000;i++)
                printf(1,"");
            getpinfo(pinfo);
            printf(1,"pinfo--\nrun time of process is %d\nwith process id %dand number of runs  %din queue number %d\n",pinfo->runtime,pinfo->pid,pinfo->num_run,pinfo->current_queue);
            exit();
        }
        else if(pid[i]>0)
        {
            status=waitx(&arg1,&arg2);
            printf(1,"pid-- %d runtime-- %d waittime-- %d",status,arg2,arg1);
        }
    }
    /*  else if(pid1>0)
        {
        continue;
        }  
        pid2=fork();
        if (!pid2 )
        {   
    //exec(argv[1], argv);
    //printf(1, "exec %s failed\n", argv[1]);
    //printf(1, "error process id =0\n");
    for(int j=0;j<10000;j++)
    printf(1,"");
    getpinfo(pinfo);
    printf(1,"pinfo --\nrun time of process is %dwith process id %dand number of runs  %din queue number %d\n",pinfo->runtime,pinfo->pid,pinfo->num_run,pinfo->current_queue);
    exit();

    }
    else if(pid2>0)
    {
    continue;
    }
    pid3=fork();

    if (!pid3 )
    {   
    //exec(argv[1], argv);
    //printf(1, "exec %s failed\n", argv[1]);
    //printf(1, "error process id =0\n");
    for(int j=0;j<10000;j++)
    printf(1,"");
    getpinfo(pinfo);
    printf(1,"pinfo--\nrun time of process is %dwith process id %dand number of runs  %din queue number %d\n",pinfo->runtime,pinfo->pid,pinfo->num_run,pinfo->current_queue);
    exit();

    }
    else if(pid3>0)
    {
    continue;
    //printf(1, "Wait Time = %d\n Run Time = %d with Status %d \n", arg1, arg2, status); 
    }*/
    exit();

}
