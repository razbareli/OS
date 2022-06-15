#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/mount.h>
#define STACK_SIZE 8192
#define MODE 0755

void error_handler(std::string const& msg){
  std::cerr<<"system error: " << msg << std::endl;
  exit(EXIT_FAILURE);
}

int child(void* args) {
  char** argv = (char**)args;
  char *hostname = argv[1];
  char *new_file_dir = argv[2];
  char *max_processes = argv[3];
  char *path_to_prog = argv[4];
  char** arguments = &argv[5];

  //change hostname
  if (sethostname(hostname, strlen(hostname)) == -1){
      error_handler ("error with changing name");
  }
  //change root directory
  if (chroot(new_file_dir) == -1){
    error_handler ("error with changing root");
  }

  // change working dir into the new root dir
  if (chdir(new_file_dir) == -1){
    error_handler ("error changing work dir to root dir");
  }

  // limit number of processes
  // create a new directory /sys/fs/cgroup/pids
  if(mkdir("sys/fs", MODE) == -1){
      error_handler ("error creating fs dir");
  }
  if(mkdir("sys/fs/cgroup", MODE) == -1){
    error_handler ("error creating cgroups dir");
  }
  if(mkdir("sys/fs/cgroup/pids", MODE) == -1){
    error_handler ("error creating pids dir");
  }

  // write to files:
  std::ofstream cgroup_procs, pids_max, notify_on_release;
  // write the process pid to cgroups.procs
  cgroup_procs.open("sys/fs/cgroup/pids/cgroup.procs");
  cgroup_procs<< "1" <<std::endl; // PID of process
  chmod("sys/fs/cgroup/pids/cgroup.procs", MODE);
  cgroup_procs.close();
  // write number of processes to limit
  pids_max.open("sys/fs/cgroup/pids/pids.max");
  pids_max<< max_processes <<std::endl; // max num of processes
  chmod ("sys/fs/cgroup/pids/pids.max", MODE);
  pids_max.close();
  // release resources
  notify_on_release.open("sys/fs/cgroup/pids/notify_on_release");
  chmod("sys/fs/cgroup/pids/notify_on_release", MODE);

  // mount the new procfs
  if (mount("proc", "/proc", "proc",0 , nullptr) == -1){
      error_handler ("error mounting procs");
  }
  // run the new program
  if (execvp(path_to_prog, arguments) == -1){
      error_handler ("error with execvp");
  }
  //release resources
  notify_on_release<<"1"<<std::endl;
  notify_on_release.close();

  return 0;
}

int main(int argc, char* argv[]) {
  //create a new process
  char* stack = (char*)malloc(STACK_SIZE);
  if (stack == nullptr){
    error_handler ("allocation failure");
  }
  int child_pid = clone(child, stack+STACK_SIZE,
                        CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD,
                        argv);
  if (child_pid == -1){
      error_handler ("error with creating child process");
  }
  wait(nullptr);

  //convert argv[2] to string
  std::string new_file_path = argv[2];

  // unmount the container
  if (umount((new_file_path + "/proc").c_str()) != 0){
      error_handler ("error with unmounting the container");
  }

  // delete file we've created
  std::string delete_cmd = "rm -rf " + new_file_path + "/sys/fs";
  system(delete_cmd.c_str());

  free(stack);
}
