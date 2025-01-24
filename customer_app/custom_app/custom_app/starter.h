#ifndef __STARTER__
#define __STARTER__

#define REAL_STACK(bytes) (bytes/sizeof(StackType_t))
#define MY_TASK_PRIO 20 // 1 works so far the best; 
extern void my_starter();

#endif