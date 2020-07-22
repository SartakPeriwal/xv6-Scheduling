#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc,char *argv[])
{
    int x=0;
    for(int i=0;i<30;i++)
    {
        int id;
        id=fork();
        if(!id)
        {
            for(int t=1;t<1000000;t+=1)
            {
                x+=t;
                x*=5;
                //printf(1,"child created with %d\n", getpid());
            }
            exit();
        }
    }
    for(int i=0;i<30;i++)
    {
        wait();
    }
    exit();

}
