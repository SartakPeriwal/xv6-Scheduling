#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "pinfo.h"

struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void ch_proc_state (void);
struct proc* returnproc(void);
void finalch(struct proc *p);





int queue[5][500];
char * name[5][500];
char * state[5][500];

    void
pinit(void)
{
    initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
    return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
    struct cpu*
mycpu(void)
{
    int apicid, i;

    if(readeflags()&FL_IF)
        panic("mycpu called with interrupts enabled\n");

    apicid = lapicid();
    // APIC IDs are not guaranteed to be contiguous. Maybe we should have
    // a reverse map, or reserve a register to store &cpus[i].
    for (i = 0; i < ncpu; ++i) {
        if (cpus[i].apicid == apicid)
            return &cpus[i];
    }
    panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
    struct cpu *c;
    struct proc *p;
    pushcli();
    c = mycpu();
    p = c->proc;
    popcli();
    return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
    static struct proc*
allocproc(void)
{
    struct proc *p;
    char *sp;

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if(p->state == UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

found:
    p->state = EMBRYO;
    p->pid = nextpid++;
    /* */    p->priority=60;
    /*  */  // p->queueno=0;
    /*  */   p->num_run=0;
    /*  */   p->ticks[0]=0;
    /*  */   p->ticks[1]=0;
    /*  */   p->ticks[2]=0;
    /*  */   p->ticks[3]=0;
    /*  */   p->ticks[4]=0;
    addtoQueue (&q[0],p);


    release(&ptable.lock);

    // Allocate kernel stack.
    if((p->kstack = kalloc()) == 0){
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe*)sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint*)sp = (uint)trapret;

    sp -= sizeof *p->context;
    p->context = (struct context*)sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint)forkret;
    p->stime=ticks;
    p->etime=0;
    p->rtime=0;

    return p;
}

//PAGEBREAK: 32
// Set up first user process.
    void
userinit(void)
{
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    TOTAL_ticks=0;
    for(int i=0;i<4;i++)
    {
        q[i].end=0;
        q[i].numofproc=0;
        q[i].ticks=(1 << i);
        q[i].priority=i;
    }

    p = allocproc();

    initproc = p;
    if((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;

    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
    int
growproc(int n)
{
    uint sz;
    struct proc *curproc = myproc();

    sz = curproc->sz;
    if(n > 0){
        if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if(n < 0){
        if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    curproc->sz = sz;
    switchuvm(curproc);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
    int
fork(void)
{
    int i, pid;
    struct proc *np;
    struct proc *curproc = myproc();

    // Allocate process.
    if((np = allocproc()) == 0){
        return -1;
    }

    // Copy process state from proc.
    if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
        kfree(np->kstack);
        np->kstack = 0;
        np->state = UNUSED;
        return -1;
    }
    np->sz = curproc->sz;
    np->parent = curproc;
    *np->tf = *curproc->tf;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for(i = 0; i < NOFILE; i++)
        if(curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;

    release(&ptable.lock);

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
    void
exit(void)
{
    struct proc *curproc = myproc();
    struct proc *p;
    int fd;

    if(curproc == initproc)
        panic("init exiting");

    // Close all open files.
    for(fd = 0; fd < NOFILE; fd++){
        if(curproc->ofile[fd]){
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0;

    acquire(&ptable.lock);

    // Parent might be sleeping in wait().
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->parent == curproc){
            p->parent = initproc;
            if(p->state == ZOMBIE)
                wakeup1(initproc);
        }
    }

    // Jump into the scheduler, never to return.
    curproc->state = ZOMBIE;
    curproc->etime=ticks;
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
    int
wait(void)
{
    struct proc *p;
    int havekids, pid;
    struct proc *curproc = myproc();

    acquire(&ptable.lock);
    for(;;){
        // Scan through table looking for exited children.
        havekids = 0;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->parent != curproc)
                continue;
            havekids = 1;
            if(p->state == ZOMBIE){
                // Found one.
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if(!havekids || curproc->killed){
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &ptable.lock);  //DOC: wait-sleep
    }
}
void run(void)
{
    struct proc * p;
    acquire(&ptable.lock);
    for(p=ptable.proc;p<&ptable.proc[NPROC];p+=1)
    {
        if(p->state == RUNNING)
        {
            p->rtime+=1;
        }
    }
    release(&ptable.lock);

}
    int
waitx(int *wtime, int *rtime)
{
    struct proc *curproc=myproc();
    int havekids;
    int pid;
    struct proc *p;
    acquire(&ptable.lock);
    for(;;){
        // Scan through table looking for zombie children.
        havekids = 0;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p+=1){
            if(p->parent != curproc)
                continue;
            havekids = 1;
            if(p->state == ZOMBIE){
                // Found one.

                *rtime = p->rtime;
                *wtime = p->etime - p->stime - p->rtime;

                // same as wait 
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->state = UNUSED;
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if(!havekids || curproc->killed){
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &ptable.lock);  //DOC: wait-sleep
    }
}

struct priorityqueue *returnQueue(void)
{
    for(int i=0;i<4;i+=1)
    {
        if(q[i].numofproc)
        {
            for(int j=0;j<q[i].end;j+=1)
            {
                if(q[i].queue[j]->state == RUNNABLE)
                {
                    return &q[i];
                }
            }
        }
    }
    return &q[0];
}
//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void addtoQueue(struct priorityqueue *pq,struct proc *p)
{
    pq->end+=1;
    pq->numofproc+=1;
    pq->queue[pq->end]=p;
    //Assign priority
    p->queueno=pq->priority;
}

void removefromQueue(struct priorityqueue *pq,struct proc *p)
{
    int loopvar =pq->end;
    for(int i=0;i< loopvar;i+=1)
    {
        pq->queue[i]=pq->queue[i+1];

    }
    pq->end--;
    pq->numofproc-=1;
}

void downqueue(struct priorityqueue *pq,struct proc *p)
{
    if(!(pq->priority))
    {
        removefromQueue(&q[0],p);
        addtoQueue(&q[1],p);
    }
    if(pq->priority==1)
    {
        removefromQueue(&q[1],p);
        addtoQueue(&q[2],p);
    }
    if(pq->priority==2)
    {
        removefromQueue(&q[2],p);
        addtoQueue(&q[3],p);

    }
    if(pq->priority==3)
    {
        removefromQueue(&q[3],p);
        addtoQueue(&q[4],p);


    }
}

void ch_proc_state(void)
{
    struct proc *p_itr;
    for(p_itr=ptable.proc;p_itr<&ptable.proc[NPROC];p_itr++)
    {
        if(p_itr->state == RUNNABLE)
        {

            p_itr->num_run++;
            /*if(p->current_queue==4)
              {
              p->wtime++;
              }*/
        }


    }
}

struct proc * returnproc(void)
{
    struct priorityqueue *pq = returnQueue();
    if(!(pq->numofproc))
    {
        return 0;
    }
    int i;
    for(i=0;i<pq->end;pq+=1)
    {
        if(pq->queue[i]->state==RUNNABLE)
        {
            struct proc *p=pq->queue[i];
            if(i)
            {
                struct proc *tmp=pq->queue[0];
                pq->queue[i]=tmp;
                pq->queue[0]=p;
            }
            return p;
        }
    }
    return 0;
}
void finalch(struct proc *p)
{
    removefromQueue(&q[4],p);
    addtoQueue(&q[0],p);
    p->queueno=0;
}
    void
scheduler(void)
{
    struct proc *p;
    //p=ptable.proc;
    struct cpu *c = mycpu();
    c->proc = 0;
    for(;;){
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
#ifdef DEFAULT

        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
            if(p->state != RUNNABLE)
                continue;

            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            p->num_run+=1;
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
        }


#else
#ifdef PBS
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
            struct proc *highPriority;
            struct proc *p_iterator;
            if(p->state != RUNNABLE)
                continue;

            highPriority = p;
            // choose one with highest priority
            for (p_iterator = ptable.proc; p_iterator < &ptable.proc[NPROC]; p_iterator+=1) {
                if(p_iterator->state != RUNNABLE)
                    continue;
                if (highPriority->priority > p_iterator->priority)
                    highPriority = p_iterator;
            }
            p = highPriority;

            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            p->num_run+=1;
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
        }

#else
#ifdef FCFS
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
            /*{
              if(p->state != RUNNABLE)
              continue;

            // Switch to chosen process.  It is the process's job
            // to release ptable.lock and then reacquire it
            // before jumping back to us.
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
            }*/
        {
            struct proc *proc_exec ;
            struct proc *itr;
            if(p->state != RUNNABLE)
                continue;

            proc_exec=p;

            // ignore init and sh processes from FCFS
            for(itr=ptable.proc;itr< &ptable.proc[NPROC];itr++)
            {

                if(itr->pid > 2)
                {
                    // if (proc_exec )
                    //{
                    if(itr->stime < proc_exec->stime && itr->state == RUNNABLE){
                        proc_exec = itr;
                    }
                    //else if(!proc_exec)
                    //  proc_exec = itr;
                }

                }
                /*if(p->pid >= 0)
                  {
                  if (proc_exec )
                  {
                  if(p->stime < proc_exec->stime)
                  proc_exec = p;
                  }
                  else if(!proc_exec)
                  proc_exec = p;
                  }
                  */
                // If I found the process which I created first and it is runnable I run it
                //(in the real FCFS I should not check if it is runnable, but for testing purposes I have to make this control, otherwise every time I launch
                // a process which does I/0 operation (every simple command) everything will be blocked

                p = proc_exec;

                if(p == 0 || p->state != RUNNABLE)
                {
                    continue;
                }
                //if(p )
                //{

                // Switch to chosen process.  It is the process's job
                // to release ptable.lock and then reacquire it
                // before jumping back to us.
                p->num_run+=1;
                c->proc = p;
                switchuvm(p);
                p->state = RUNNING;

                swtch(&(c->scheduler), p->context);
                switchkvm();

                // Process is done running for now.
                // It should have changed its p->state before coming back.
                cprintf("FCFS running with pid %d and cpu %d\n",p->pid,c->apicid);
                c->proc = 0;

            }
#else
#ifdef MLFQ
            for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
            {
                if(p!=returnproc())
                {
                    continue;
                }
                struct priorityqueue *queue =returnQueue();

                //int queueprior=p->queueno;
                int count=0;
                while(p->state ==RUNNABLE && count < queue->ticks)
                {
                    c->proc = p;

                    switchuvm(p);
                    p->state = RUNNING;

                    swtch(&(c->scheduler), p->context);
                    switchkvm();

                    // Process is done running for now.
                    //         // It should have changed its p->state before coming back.
                    c->proc = 0;
                    count++;
                    TOTAL_ticks+=1;
                    if(TOTAL_ticks<NTICKS)
                    
                        TOTAL_ticks+=1;
                }
                p->ticks[p->queueno]+= 1;
                ch_proc_state();

                 if (p->state == RUNNABLE && (p->queueno == 1 || p->queueno == 0))
                 {
                             downqueue(queue, p);
                 }
                if(p->state == RUNNABLE && p->queueno ==4 )
                {
                    finalch(p);
                }
                if(p->state ==SLEEPING)
                {
                    removefromQueue(queue,p);
                    addtoQueue(queue,p);
                }

            }
        



#endif
#endif
#endif
#endif

            release(&ptable.lock);
            }
        }



        // Enter scheduler.  Must hold only ptable.lock
        // and have changed proc->state. Saves and restores
        // intena because intena is a property of this
        // kernel thread, not this CPU. It should
        // be proc->intena and proc->ncli, but that would
        // break in the few places where a lock is held but
        // there's no process.
        void
            sched(void)
            {
                int intena;
                struct proc *p = myproc();

                if(!holding(&ptable.lock))
                    panic("sched ptable.lock");
                if(mycpu()->ncli != 1)
                    panic("sched locks");
                if(p->state == RUNNING)
                    panic("sched running");
                if(readeflags()&FL_IF)
                    panic("sched interruptible");
                intena = mycpu()->intena;
                swtch(&p->context, mycpu()->scheduler);
                mycpu()->intena = intena;
            }

        // Give up the CPU for one scheduling round.
        void
            yield(void)
            {
                acquire(&ptable.lock);  //DOC: yieldlock
                myproc()->state = RUNNABLE;
                sched();
                release(&ptable.lock);
            }

        // A fork child's very first scheduling by scheduler()
        // will swtch here.  "Return" to user space.
        void
            forkret(void)
            {
                static int first = 1;
                // Still holding ptable.lock from scheduler.
                release(&ptable.lock);

                if (first) {
                    // Some initialization functions must be run in the context
                    // of a regular process (e.g., they call sleep), and thus cannot
                    // be run from main().
                    first = 0;
                    iinit(ROOTDEV);
                    initlog(ROOTDEV);
                }

                // Return to "caller", actually trapret (see allocproc).
            }

        // Atomically release lock and sleep on chan.
        // Reacquires lock when awakened.
        void
            sleep(void *chan, struct spinlock *lk)
            {
                struct proc *p = myproc();

                if(p == 0)
                    panic("sleep");

                if(lk == 0)
                    panic("sleep without lk");

                // Must acquire ptable.lock in order to
                // change p->state and then call sched.
                // Once we hold ptable.lock, we can be
                // guaranteed that we won't miss any wakeup
                // (wakeup runs with ptable.lock locked),
                // so it's okay to release lk.
                if(lk != &ptable.lock){  //DOC: sleeplock0
                    acquire(&ptable.lock);  //DOC: sleeplock1
                    release(lk);
                }
                // Go to sleep.
                p->chan = chan;
                p->state = SLEEPING;

                sched();

                // Tidy up.
                p->chan = 0;

                // Reacquire original lock.
                if(lk != &ptable.lock){  //DOC: sleeplock2
                    release(&ptable.lock);
                    acquire(lk);
                }
            }

        //PAGEBREAK!
        // Wake up all processes sleeping on chan.
        // The ptable lock must be held.
        static void
            wakeup1(void *chan)
            {
                struct proc *p;

                for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
                    if(p->state == SLEEPING && p->chan == chan)
                        p->state = RUNNABLE;
            }

        // Wake up all processes sleeping on chan.
        void
            wakeup(void *chan)
            {
                acquire(&ptable.lock);
                wakeup1(chan);
                release(&ptable.lock);
            }

        // Kill the process with the given pid.
        // Process won't exit until it returns
        // to user space (see trap in trap.c).
        int
            kill(int pid)
            {
                struct proc *p;

                acquire(&ptable.lock);
                for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
                    if(p->pid == pid){
                        p->killed = 1;
                        // Wake process from sleep if necessary.
                        if(p->state == SLEEPING)
                            p->state = RUNNABLE;
                        release(&ptable.lock);
                        return 0;
                    }
                }
                release(&ptable.lock);
                return -1;
            }

        //PAGEBREAK: 36
        // Print a process listing to console.  For debugging.
        // Runs when user types ^P on console.
        // No lock to avoid wedging a stuck machine further.
        void
            procdump(void)
            {
                static char *states[] = {
                    [UNUSED]    "unused",
                    [EMBRYO]    "embryo",
                    [SLEEPING]  "sleep ",
                    [RUNNABLE]  "runble",
                    [RUNNING]   "run   ",
                    [ZOMBIE]    "zombie"
                };
                int i;
                struct proc *p;
                char *state;
                uint pc[10];

                for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
                    if(p->state == UNUSED)
                        continue;
                    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
                        state = states[p->state];
                    else
                        state = "???";
                    cprintf("%d %s %s", p->pid, state, p->name);
                    if(p->state == SLEEPING){
                        getcallerpcs((uint*)p->context->ebp+2, pc);
                        for(i=0; i<10 && pc[i] != 0; i++)
                            cprintf(" %p", pc[i]);
                    }
                    cprintf("\n");
                }
            }
        int ps(void)
        {
            struct proc *p_iterator;
            sti();//enable interrrupts
            acquire(&ptable.lock);
            cprintf("Proc name \t Process-pid \t \t State \t \t \t Priority \n\n");
            for(p_iterator = ptable.proc;p_iterator < &ptable.proc[NPROC];p_iterator++)
            {
                if(p_iterator->state == RUNNING || p_iterator->state == SLEEPING || p_iterator->state == RUNNABLE )
                {
                    if(p_iterator->state == SLEEPING)
                    {
                        cprintf("%s \t \t \t%d \t \t SLEEPING \t \t %d \n",p_iterator->name,p_iterator->pid,p_iterator->priority);
                    }  
                    if(p_iterator->state == RUNNING)
                    {
                        cprintf("%s \t \t \t%d \t \t RUNNING \t \t %d \n",p_iterator->name,p_iterator->pid,p_iterator->priority);
                    }
                    if(p_iterator->state == RUNNABLE)
                    {
                        cprintf("%s \t \t \t%d \t \t RUNNABLE \t \t %d \n",p_iterator->name,p_iterator->pid,p_iterator->priority);
                    }
                }
            }
            release(&ptable.lock);
            return 1;
        }
        int set_priority(int pid,int priority)
        {
            struct proc * p_iterator;
            acquire(&ptable.lock);
            for(p_iterator=ptable.proc;p_iterator< &ptable.proc[NPROC];p_iterator++)
            {
                if(p_iterator->pid==pid)
                {
                    cprintf("Previous priority\nwith PID%d Priority%d\n",p_iterator->pid,p_iterator->priority);
                    p_iterator->priority=priority;
                    cprintf("Changed priority\nwith PID%d Priority%d\n",p_iterator->pid,priority);
                }
            }
            release(&ptable.lock);
            return 1;

        }
        int getpinfo(struct proc_stat *p)
        {
            struct proc *curproc = myproc();
            p->runtime=curproc->rtime;
            p->pid =curproc->pid;
            p->current_queue=curproc->queueno;
            p->num_run=curproc->num_run;
            p->ticks[0]=curproc->ticks[0];
            p->ticks[1]=curproc->ticks[1];
            p->ticks[2]=curproc->ticks[2];
            p->ticks[3]=curproc->ticks[3];
            p->ticks[4]=curproc->ticks[4];
            return 1;
        }
