#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main (int argc,char *argv[])
{
    int arg1;
    int arg2;
    int pid;
    int status=4; 
    pid = fork();
    if (!pid )
    {   
        exec(argv[1], argv);
        printf(1, "exec %s failed\n", argv[1]);
        printf(1, "error process id =0\n");
        exit();

    }
    else if(pid>0)
    {
        status = waitx(&arg1, &arg2);
    }  
    printf(1, "Wait Time = %d\n Run Time = %d with Status %d \n", arg1, arg2, status); 
    exit();

}
