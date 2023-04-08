//Libraries used
//<fcntl.h> needed for file opening snippet in redirect function 
#define _GNU_SOURCE
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

//External environment 
extern char **environ;

//Function to add a file redirect for the CNET_ID mode
int redirect(char * filename)
{
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
      perror("open");
      return EXIT_FAILURE;
    }
  dup2(fd, STDIN_FILENO);
  close(fd);
  return EXIT_SUCCESS;
}

//Function to check if all the child processes have been terminated successfully before moving on with the parent process
int check_child_exit(int child_status, pid_t wpid, int line_no){
    for (int i = 0; i < line_no; i++) { // Parent 
        wpid = wait(&child_status);
        if (WIFEXITED(child_status)){
            continue;
        }
        else{
            printf("Child %d terminated abnormally\n", wpid);
        }
    }
    return 0;
}

//Fork function (all child processes' functionalities are inside this function)
void forkfunc(char * name, char * redirectfile, pid_t pid) {
    int nargc = 2; 
    char * myargv[] = {"../p1/./du",name,NULL}; 
    if ((pid = fork()) == 0) {   
        redirect(redirectfile);
        execve(myargv[0], myargv, environ); 
        _exit(EXIT_FAILURE); 
    }
}

//TO check if file argument is provided, and to check if the provided argument is valid
void file_check(char * filename){
    if (filename == NULL){
        printf("File argument not provided.\n");
        exit(EXIT_FAILURE);
    } 

    if (access(filename, F_OK) != 0) {
        printf("File does not exist.\n");
        exit(EXIT_FAILURE);
    }
}

//Main func
int main(char argc, char *argv[])
{
    file_check(argv[1]);
    pid_t wpid, pid;
    char *line = NULL;
    int i, child_status;
    long int len = 0;
    int line_no = 0;
    long nRead = getline(&line, &len, stdin);
    //Reading lines one by one until empty line is encountered
    while ( nRead != -1) 
    {
        line_no++;
        line[strcspn(line, "\n")] = 0;
        forkfunc(line,argv[1],pid);
        free(line);
        line = NULL;
        nRead = getline(&line, &len, stdin);
    }
    //If all child processes have exited successfully, print "Done." 
    if (check_child_exit(child_status, wpid, line_no) == 0){
        printf("Done.\n");
    }
    return 0;
}

