#ifndef exec_progH
#define exec_progH
#include <unistd.h>                                            
#include <sys/types.h>
#include <sys/wait.h>                                          
#include <stdio.h>
#include <stdlib.h>

int exec_prog(const char *argv, pid_t& Pid);
int wait_prog(pid_t Pid, int Timeout);

#endif
