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
bool unmountMode = false;
bool mountMode = false;
bool versionMode = false;
bool helpMode = false;
#define MAX_ARG   (255)
#define MKFS_EXT3 ("ext3 ")
#define MKFS_EXT2 ("ext2 ")
std::string fsProg = MKFS_EXT3;

std::string _version = "1.0.0";

#define CL_NONE      "\033[m" 
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"

#define CL_PARENT     LIGHT_CYAN

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

  FILE* pipe = popen("sudo parted -l", "r");
  
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
int MountUnmountDevices(std::vector<std::string >& DeviceNames, bool Mount)
{
  int dev_cnt = 0;
  int umount_cnt = 0;
  //char* cmd = new char[256];
  std::string cmd_line;

  dev_cnt = DeviceNames.size();
  
  if (Mount) {
    printf("%sUnmount %d devices", CL_PARENT, dev_cnt);
  }
  else {
    printf("%sMount %d devices", CL_PARENT, dev_cnt);
  }  
  printf("%s", CL_NONE);

  for (int dev = 0; dev < dev_cnt; ++dev) {
    //printf("[Debug] unmount device %s\n", devNames[dev].c_str());        
    //sprintf(cmd, "umount %s", devNames[dev].c_str());
    if (!Mount) {            
      cmd_line = "umount " + DeviceNames[dev];
    }
    else {
      cmd_line = "udisksctl mount -b " + DeviceNames[dev];
    }
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

  printf("%s%d devices are starting to formatted as type %s\n%s", CL_PARENT, dev_cnt, fsProg.c_str(), CL_NONE);
  
  for (int dev = 0; dev < dev_cnt; ++dev) {
#if MULTI_PROCESS == 1
    pid = fork();
#else
    pid = 0;
#endif
    if (pid == 0) {
      printf("Created device %d process. PID=%d\n", dev, getpid());        
      cmd_line =  "yes | sudo mkfs." + fsProg + DeviceNames[dev];

      if (fsProg.find("fat") != std::string::npos) {
        cmd_line += " -I";
      }

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
  printf("%sAll format process done\n", CL_PARENT);

  clock_gettime(CLOCK_REALTIME, &end);

  int sec = end.tv_sec - start.tv_sec;

  printf("%sEclipsed: %d (minuts) and %d (seconds)\n", CL_PARENT, sec / 60, sec % 60);

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

  bool indicateType = false;

  for (int i = 1; i <argc; ++i) {
    if (argv[i][0] != '-') {
      if (!indicateType) {
        err = std::string("Invalid argument: ") + argv[i];
        perror(err.c_str());   
      }
    }     
    std::string arg = indicateType ? argv[i] :  GetArgument(argv[i]);
    
    if (indicateType) {
      fsProg = std::string(argv[i]) + " ";
      continue;
    }

    listMode = false;
    unmountMode = false;
    mountMode = false;
    versionMode = false;
    helpMode = false;    

    if (arg == "l") {
      listMode = true;
    }
    else if (arg == "u") {
      unmountMode = true;
    }
    else if (arg == "m") {
      mountMode = true;
    }
    else if (arg == "f") {
      indicateType = true;
    }    
    else if (arg == "-help" || arg == "h") {
      helpMode = true;
    }
    else if (arg == "-version" || arg == "v") {
      versionMode = true;
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
  printf("%s", CL_NONE);
}

/*
 *  Version and copyright statement.
 */
void VersionStatement()
{
  std::string version = 
          "\nmpfs (1.0.0) Copyright (c) EmBestor Technology Inc.\n"
          "This is free and unencumbered software released into the public domain.\n\n"

          "Anyone is free to copy, modify, publish, use, compile, sell, or "
          "distribute this software, either in source code form or as a compiled "
          "binary, for any purpose, commercial or non-commercial, and by any means.\n\n";

  printf("%s", version.c_str());
}

/*
 *  Print help document
 */
void HelpDocument()
{
  std::string doc =
      "\nThis program is used for taking some operation on single or multiple usb storage device "
      "which named in the specific model \n(Generic CRM01 Reader).\n\n"
      "The open source software ""GNU parted"" is used for grabbing "
      "storage device information and extracted for the target devices.\n"
      "The tool ""mkfs"" of util-linux software is called for taking format to the target devices.\n\n"
      "If more than 1 device found, individual process will be forked for each target device.\n\n" 

      "Just call ""mpfs"" WITHOUT ANY option for doing DEFAULT FORMAT (ext3) automatically.\n"
      "Or adding option for the other services, just refer to the folloing:\n\n"
      "  mpfs [option] [format type]\n\n" 

      "   option:\n"
      "     -h or --help: Call for showing this document.\n"
      "     -v or --version: Display version and copyright information.\n"
      "     -l: List all target devices that found as ''Generic CRM01 Reader''.\n"
      "     -u: Unmount target devices (Use umount command).\n"
      "     -m: Mount target devices (Use udisksctl mount command).\n" 
      "     -f: Indicate format type by the following [format type] option.\n\n"

      "   format type: According to mkfs provided. eq. ext2, ext3, ext4, ntfs, fat, vfat..etc\n"
      "                for example:\n"
      "                 $ mpfs -f ext4\n\n";

  printf("%s", doc.c_str());
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

  ParseInput(argc, argv);
  
  /* Just print version or help then leave */
  if (versionMode) {
    VersionStatement();
    return 0;
  }
  if (helpMode) {
    HelpDocument();
    return 0;
  }       

  /* Execute parted command to get the partition table */                  
  result = GetPartedResult();  
  dev_cnt = ParseDeviceNamesFromPartedResult(result, devNames);  
  assert(dev_cnt == devNames.size());
  
  printf("%sDevices found: [%d devices]\n", YELLOW, dev_cnt);
  ListDevicesFound(devNames);
  
  if (mountMode) {    
    MountUnmountDevices(devNames, true);
    return 0;
  }

  if (listMode) {
    return 0;
  }
  /* Unmount device first */  
  MountUnmountDevices(devNames, false);
  
  if (unmountMode) {
    return 0;
  }
  /* Becuase after unmount, the device name may be changed
     Need to get again for updating */
  result = GetPartedResult();

  devNames.clear();
  dev_cnt = ParseDeviceNamesFromPartedResult(result, devNames);   

  int pid = CreateFormatProcess(devNames);
 
  return 0;
}
