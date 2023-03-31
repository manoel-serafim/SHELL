#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/wait.h>

#define RESET "\033[0m"
#define RED "\033[31m"
#define CYN "\033[36m"

#define MAX_CMD 1024
#define MAX_ARG 1024

int parse(char * line, char ** argv, char * delimiter) {
  int argc = 0;//counter of components
  char * token = strtok(line, delimiter);//token holder
  while (token != NULL) {
    argv[argc++] = token; //store token
    token = strtok(NULL, delimiter);
  }
  argv[argc] = NULL;
  return argc;
}

void execute(char ** argv, int input_fd, int output_fd) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork()");
    exit(EXIT_FAILURE);
  } else if (pid == 0) { // child process
    if (input_fd != STDIN_FILENO) {
      dup2(input_fd, STDIN_FILENO);
      close(input_fd);
    }
    if (output_fd != STDOUT_FILENO) {
      dup2(output_fd, STDOUT_FILENO);
      close(output_fd);
    }
    if (execvp(argv[0], argv) < 0) {
      perror("execvp()");
      exit(EXIT_FAILURE);
    }
  } else { // parent process
    int status;
    waitpid(pid, & status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      fprintf(stderr, "Command failed with exit status %d\n", WEXITSTATUS(status));
    }
  }
}


void cmd_handler(int cmd_counter, char** commands){
    int input_fd = STDIN_FILENO;// used as holder of the previous fd[0]
    for (int i = 0; i < cmd_counter; i++) {
        char * args[MAX_ARG];// arguments of each command
        int num_args = parse(commands[i], args, " \t\n");// get cmd args
        
        if (num_args == 0) {// if no command or argument is found in args
          continue;
        }
        int output_fd = (i == cmd_counter - 1) ? STDOUT_FILENO : -1; // output will be equal to STDOUT_FILENO if it is the last command 
        
        
        int fd[2];// pipe def
        if (i < cmd_counter - 1) {// in last command the pipe will not execute
          if (pipe(fd) < 0) {// pipe creator
            perror("pipe()");
            break;
          }
          output_fd = fd[1]; // cmd will write into fd[1] in cases where
        }
        execute(args, input_fd, output_fd); // cmd write based on each
        if (i < cmd_counter - 1) {//in the last command this step will not be necessary
          close(fd[1]);
          input_fd = fd[0]; //next command will get input from the last
        }
      } //for every command
}



int main(int argc, char ** argv) {
  if (argc == 1) {//if shell is summoned
  
  
    char * line = NULL; // line pointer for initial cmd
    char * cwd = (char * ) malloc(sizeof(char) * PATH_MAX); //CWD
    size_t bufsize = 0;//getline buffer
    
    while (1) {
      if (getcwd(cwd, PATH_MAX) == NULL) {//get CWD and store at cwd
        perror("getcwd() error");
        continue;
      }
      
      printf(RED "["RESET "~%s"RED "]$>", cwd);//print indicator
      
      if ((int) getline( &line, &bufsize, stdin) < 0) {//get user input:
        perror("getline()");
        continue;
      }
      
      char * commands[MAX_CMD]; // holder of each cmd block
      int num_commands = parse(line, commands, "\"|\"");
      
      if (num_commands == 0) {//if no command was received continue
        continue;
      }
      if (strcmp(*commands, "exit\n") == 0) {//if received exit break
        break;
      }
      
     cmd_handler(num_commands, commands);
      
      
    }
    free(cwd);
    free(line);
  } //if(argc==1)
  else {
    char * commands[MAX_CMD];
    int num_commands = parse(*(argv+1), commands, "\"|\"");
    if (num_commands == 0) {
      return 0;
    }
    cmd_handler(num_commands, commands);
    
  }
  return 0;
  
}
