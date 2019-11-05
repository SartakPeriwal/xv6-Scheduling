#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc,char *argv[])
{
    for(int i=0;i<10;i++)
    {
        int id;
        id=fork();
        if(!id)
        {
            for(int t=0;t<10;t+=1)
            {
                //printf(1,"child created with %d\n", getpid());
            }
            exit();
        }
    }
    for(int i=0;i<10;i++)
    {
        wait();
    }
    exit();

}
