#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc ,char * argv[])
{
    if(argc<=2)
    {
        printf(1,"arguments supplied incorrectly");
    }
    int priority=atoi(argv[2]);
    int pid=atoi(argv[1]);
    if(priority<0 || priority >100)
    {
        printf(1,"range of priority not correctly ");
    }
    set_priority(pid,priority);
    exit();
}
