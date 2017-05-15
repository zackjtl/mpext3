#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

pid_t child_pid, wpid;
int status = 0;
bool listMode = false;
#define MAX_ARG   (255)
#define MKFS_EXT3 ("mkfs.ext3 ")
#define MKFS_EXT2 ("mkfs.ext2 ")
std::string fsProg = MKFS_EXT3;

#define MULTI_PROCESS 1

/*
 *  Returns next position of change line sign if the given start position is valid.
 *  If change line sign not found, returns npos.
 */
std::size_t ToNextLine(const std::string& Input, std::size_t Pos)
{
  if (Pos == std::string::npos) {
    return Pos;
  }      
  std::size_t pos = Input.find_first_of("\n", Pos);

  if (pos != std::string::npos) {
    return pos + 1;
  }
  return pos;
}
/*
 *  Get string of the result of parted command.
 */
std::string GetPartedResult()
{
  char buffer[128];
  std::string result = "";

  FILE* pipe = popen("parted -l", "r");
  
  if (!pipe) {
    perror("popen() failed!");
  }
  try {
    while (!feof(pipe)) {
      if (fgets(buffer, 128, pipe) != NULL) {
        result += buffer;
      }
    }
  }
  catch (...) {
    pclose(pipe);
    throw;
  }
  pclose(pipe);

  return result;
}
/*
 *  Parse parted result and search CRM01 Reader device.
 *  Initial the format command with the device name.
 */
int ParseDeviceNamesFromPartedResult(const std::string& result, std::vector<std::string >& DeviceNames)
{
  int dev_cnt = 0;

  std::size_t pos = result.find("Generic CRM01 SD Reader");
  pos = ToNextLine(result, pos);
  
  while (pos != std::string::npos) {
    //printf("Current pos = %d\n", (int)pos);
    std::size_t sub_pos = result.find("/dev/", pos + 1);

    if (sub_pos != std::string::npos) {     
      std::size_t endl_pos = result.find("\n", sub_pos + 1);
      std::size_t end_pos = result.find("：", sub_pos + 1);

      if (endl_pos != std::string::npos) {
        std::string line = result.substr(sub_pos, endl_pos - sub_pos);
        //printf("Got line: %s\n", line.c_str());
      }

      if (end_pos == std::string::npos ||
          ((endl_pos != std::string::npos) && (end_pos > endl_pos))) {

        end_pos = result.find(":", sub_pos + 1);

        if (end_pos == std::string::npos ||
            ((endl_pos != std::string::npos) && (end_pos > endl_pos))) {
          //perror("can't find end of device name");
          break;
        } 
      }
      std::string sub = result.substr(sub_pos, end_pos - sub_pos);
      //printf("%s\n", sub.c_str());

      DeviceNames.push_back(sub);
      ++dev_cnt;

      pos = end_pos;
    }     
    pos = result.find("Generic CRM01 SD Reader", pos + 1);         
    pos = ToNextLine(result, pos);
  }
  return dev_cnt;
}
/*
 * Should un-mount devices before doing mkfs.
 */
int UnmountDevices(std::vector<std::string >& DeviceNames)
{
  int dev_cnt = 0;
  int umount_cnt = 0;
  //char* cmd = new char[256];
  std::string cmd_line;

  dev_cnt = DeviceNames.size();

  for (int dev = 0; dev < dev_cnt; ++dev) {
    //printf("[Debug] unmount device %s\n", devNames[dev].c_str());        
    //sprintf(cmd, "umount %s", devNames[dev].c_str());
    cmd_line = "umount " + DeviceNames[dev];
    try {
      if (-1 != system(cmd_line.c_str())) {
        ++umount_cnt;
      }
    }
    catch (...) {
      printf("Available device to un-mount\n");
    }
  }
  return umount_cnt;
}
/*
 *  Create processes to execute mkfs format devices.
 */
int CreateFormatProcess(std::vector<std::string >& DeviceNames)
{
  int dev_cnt = DeviceNames.size();
  //char* cmd = new char[256];
  std::string cmd_line;
  struct timespec start, end;
  clock_gettime(CLOCK_REALTIME, &start);  

  assert(dev_cnt > 0); 
  /* Becuase after unmount, the device name may be changed
     Need to get again for updating */

  /* Execute format command */
  pid_t pid = 65534;
  
  for (int dev = 0; dev < dev_cnt; ++dev) {
#if MULTI_PROCESS == 1
    pid = fork();
#else
    pid = 0;
#endif
    if (pid == 0) {
      printf("Try to create device %d process. PID=%d\n", dev, getpid());        
      cmd_line =  "yes | " +  fsProg + DeviceNames[dev];

      if (system(cmd_line.c_str()) == -1) {
        perror("Execute error");
      }
      printf("Device %s format done (PID:%d)\n", DeviceNames[dev].c_str(), getpid());     
#if MULTI_PROCESS == 1 
      return pid;
#endif
    }    
    else if (pid == -1) {
      printf("Execute process for device %s error\n", DeviceNames[dev].c_str());      
    }
    else {
    }
  }
#if MULTI_PROCESS == 1 
  int status;
  while (pid = waitpid(-1, &status, 0) != -1) {
  }
#endif
  printf("[Parrent Process]: Process all finished\n");

  clock_gettime(CLOCK_REALTIME, &end);

  int sec = end.tv_sec - start.tv_sec;

  printf("second = %d\n", sec);

  return pid;
  //waitpid(-1, NULL, 0);
}
/*
 *  Extract argument from argument list.
 */
std::string GetArgument(char* arg)
{
  std::string ret = "";        
  if (std::strlen(arg) > MAX_ARG) {
    perror("Argument too long to parse");
  }
  char temp[MAX_ARG];
  strcpy(temp, (char*)&arg[1]); 
  ret = temp;

  return ret;
}
/*
 *  Parse argument to change variables.
 */
void ParseInput(int argc, char* argv[])
{
  if (argc == 0) {
    listMode = false;
    fsProg = MKFS_EXT3;
    return;
  }
  std::string err;

  for (int i = 1; i <argc; ++i) {
    if (argv[i][0] != '-') {
      err = std::string("Invalid argument: ") + argv[i];
      perror(err.c_str());   
    }    
    std::string arg = GetArgument(argv[i]);  

    listMode = false;

    if (arg == "l") {
      listMode = true;
    }
    if (arg == "ext3") {
      fsProg = MKFS_EXT3;    
    } 
    else if (arg == "ext2") {
      fsProg = MKFS_EXT2;
    }   
  }
}

/*
 *  List devices found. (CRM01 Reader)
 */
void ListDevicesFound(std::vector<std::string >& DeviceNames)
{
  int dev_cnt = DeviceNames.size();
  
  for (int dev = 0; dev <dev_cnt; ++dev) {
    printf(" %s\n", DeviceNames[dev].c_str());
  }
}

/*
 *  The main program, parse partition information from parted command, 
 *  find CRM01 Reader device and execute mkfs to format the device.
 */
int main(int argc, char* argv[])
{
  int dev_cnt = 0;
  std::vector<std::string > devNames;
  std::string result; 

  /* Parse input arguments to modify behavior */
  ParseInput(argc, argv);

  result = GetPartedResult();
  dev_cnt = ParseDeviceNamesFromPartedResult(result, devNames);  
  assert(dev_cnt == devNames.size());

  printf("Devices found: [%d devices]\n", dev_cnt);
  ListDevicesFound(devNames);

  if (listMode) {
    return 0;
  }
  /* Unmount device first */  
  UnmountDevices(devNames);

  /* Becuase after unmount, the device name may be changed
     Need to get again for updating */
  result = GetPartedResult();

  devNames.clear();
  dev_cnt = ParseDeviceNamesFromPartedResult(result, devNames);   

  int pid = CreateFormatProcess(devNames);
 
  return 0;
}
