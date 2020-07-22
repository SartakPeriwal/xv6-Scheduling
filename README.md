```C
logic for waitx
add the syscall in all the given required files like 
usys.S
user.h
 syscall.h
 syscall.c
 defs.h
 sysproc.c

use the same command as for wait and only change while calling in syscall.c

pinfo -:
make struct give same files changes as for waitx 
initialize in alloc proc and update values whenever necessary
and call the function using pinfo

FCFS-:
itreate through the proc table and store the process with minimum start time so that it will be the process which needs to be allocated CPU;

PBS-:
similar as FCFS only check which priority is lowest and give cpu

to check
run check &;check &
now use ps
change priority of runnable process to less than that of running process 
use ps 
running states switch
MLFQ
make 5 queues and check which queue to give the process 
write 4 functions return queue
add queue
remove queue
down queue
final_ch 
which allocate to a process to a queue or remove it from a queue or change its queue or move from q4 to q0;
now when iterating through the process table check to iterate through the process returned store all the values in process pointers; and keep checking the priority if it has to be added to a different queue or not;
comparison MLFQ is the fastest in all
PBS is similar to FCFS but gives cpu to priority
FCFS
Wait Time = 2
 Run Time = 33 with Status 4
PBS
Wait Time = 4
 Run Time = 25
RR is impartial method of scheduling
Wait Time = 7
 Run Time = 23
```

