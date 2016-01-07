#include <string>
#include <thread>
#include <iomanip>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#define READ 0
#define WRITE 1

FILE *popen2(std::string command, std::string type, int &pid,
             std::string low_lvl_user) {
  pid_t child_pid;
  int fd[1];
  pipe(fd);
  if ((child_pid = fork()) == -1) {
    perror("fork");
    exit(1);
  }
  if (child_pid == 0) { // child begins
    if (type == "r") {
      close(fd[READ]); // Close the READ
      dup2(fd[WRITE], 1); // Redirect stdout to pipe
    } else {
      close(fd[WRITE]); // Close the WRITE
      dup2(fd[READ], 0); // Redirect stdin to pipe
    }
    if (getuid() == 0) {
      execl("/bin/su", "su", "-c", "/bin/sh", "-c", command.c_str(),
            low_lvl_user.c_str(),
            NULL); // fixes not being able to reap suid 0 processes
    } else {
      execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL); // runs it all
    }
    exit(0);
  } else {
    if (type == "r") {
      close(fd[WRITE]); // Close the WRITE
    } else {
      close(fd[READ]); // Close the READ
    }
  }
  pid = child_pid;
  if (type == "r") {
    return fdopen(fd[READ], "r");
  }
  return fdopen(fd[WRITE], "w");
}

int pclose2(FILE *fp, pid_t pid) // close it so we don't fuck outselves
{
  int stat;
  fclose(fp);
  while (waitpid(pid, &stat, 0) == 0) {
    if (errno != EINTR) {
      stat = -1;
      break;
    }
  }
  return stat;
}
