#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int fork_id = fork();
    if (!fork_id ) {
        double d = 2;
        double x = 0;
        for (int i = 1; i <= 300; ++i) 
        {
                for (int k = 0; k <= 2000000; k += d) 
                {
                    for(int l=0;l<2;l++)
                        x+= 107;
                    k+=d;

                }


        }
        printf(1, "Test function ended\n");

    }
    exit();

}
