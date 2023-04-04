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

int execute(char ** argv, int input_fd, int output_fd, int bk) {

  
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
    
    if(bk == 0){
        waitpid(pid, &status, 0);
    }
     // status will be used for || based on status run next will change
    
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
      fprintf(stderr, "Command failed with exit status %d\n", WEXITSTATUS(status));
    }
    
    
    return status;
  }
}// return status of executed cmd


void cmd_handler(int pipe_counter, char** pcommands){ //commands and cmd counter are based on the number of pipes
    
    /*
    //get conditional directives in pcommands
    //AND &&
    char* ccommands[MAX_ARG];
    int and_counter = parse(*(pcommands), ccommands, "\"&&\"";
    
    int or_counter = parse(*(ccommands), ccommands, "\"||\"";
    
    */
    
    
    int input_fd = STDIN_FILENO;// used as holder of the previous fd[0]
    int lstat = 0;
    int bk = 0;
    for (int i = 0; i < pipe_counter; i++) {
        bk =0;
        
        char * args[MAX_ARG];// arguments of each command
        int num_args = parse(pcommands[i], args, " \t\n");// get cmd args
        
        if (num_args == 0) {// if no command or argument is found in args
          continue;
        }
        
        
        
        int output_fd = (i == pipe_counter - 1) ? STDOUT_FILENO : -1; // output will be equal to STDOUT_FILENO if it is the last command 
        int fd[2];// pipe def
        if (i < pipe_counter - 1) {// in last command the pipe will not execute
          if (pipe(fd) < 0) {// pipe creator
            perror("pipe()");
            break;
          }
          output_fd = fd[1]; // cmd will write into fd[1] when not in last command
        }
        
        
        if( **(args+num_args-1) == '&'){
        
            bk = 1;
            output_fd = STDOUT_FILENO;//1
            *(args+num_args-1) = NULL;
        }
        
        lstat = execute(args, input_fd, output_fd,bk); // cmd write based on each
        // lstat = status of last cmd
        
        
        
        
        if (i < pipe_counter - 1) {//in the last command this step will not be necessary
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
      fflush(stdout);// erase any buffered output
      
      
      if ((int) getline( &line, &bufsize, stdin) < 0) {//get user input:
        perror("getline()");
        continue;
      }
      
      
      char * commands[MAX_CMD]; // holder of each cmd block
      int num_commands = parse(line, commands, "|");//first separation will happen with pipes
      
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
