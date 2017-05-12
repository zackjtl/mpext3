#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include "exec_prog.h"


/*
 *  Execute process with creating a new pid.
 *  Will get the created pid by the second argument reference.
 */
int exec_prog(const char *argv, pid_t& Pid)
{
  //char** remain_argv = ((char**)argv) + 1;

  Pid = fork();

  if (0 != Pid) {
    printf("PID: %d\n", Pid);
    printf("Will execute program: %s\n", argv);
    if (-1 == system(argv)) {            
    //if (-1 == execve(argv[0], remain_argv, NULL)) {
      perror("cbild process execve failed");
      return -1;         
    }
  }
  /*
  else {
    perror("fork process failed");
    return -1;
  }*/
  /*
  if (Pid == 0) {
    return 0;
  }

  int timeout = 1000;
  int status;

  while (0 == waitpid(Pid, &status, WNOHANG)) {
    if (--timeout < 0) {
      perror("timeout");
      return -1;      
    }
    sleep(1);
  }
  printf("WEXITSTATUS %d WIFEXITED %d [stauts %d], pid=%d\n",
         WEXITSTATUS(status), WIFEXITED(status), status, Pid);

  if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
    perror("Process failed, halt system");
    return -1;    
  }
  */


  return 0;
}

/*  
 *  Input pid to wait for time of Timeout.
 *
 */
#if 1
int wait_prog(pid_t Pid, int Timeout)
{
  int timeout = Timeout;
  int status;

  if (timeout <= 0) {
    return 0;
  }
  while (0 == waitpid(Pid, &status, WNOHANG)) {    
    if (Timeout > 0) {
      timeout -= 10;

      if (timeout < 0) {
        perror("timeout");
        return -1;      
      }
    }
    sleep(10);
  }
  printf("WEXITSTATUS %d WIFEXITED %d [stauts %d]\n",
         WEXITSTATUS(status), WIFEXITED(status), status);

  if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
    perror("Process failed, halt system");
    return -1;    
  }
  return 0;
}    
#endif
