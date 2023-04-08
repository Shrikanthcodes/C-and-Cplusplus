/*
 * tsh - A tiny shell program with job control
 *
 * CNET: Shrikanth
 * 
 */



//Libraries used
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */
#define MAXHISTORY   10

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */
char * username;            /* The name of the user currently logged into the shell */
struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
int no_of_history = 0;      /*History index count*/
char history[MAXHISTORY][MAXLINE];  /*The array to save and update history*/
/* End global variables */



/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);
char * login();
void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

// Additional functions that I have implemented

void delete_proc_file(int pid);
void modify_history(char * command);
void save_history();

int shell_pid; //To store shell pid



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    username = login();

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}

/*
 * login - Performs user authentication for the shell
 *
 * See specificaiton for how this function should act
 *
 * This function returns a string of the username that is logged in
 */
char * login() {
    char username[32];
    char password[32];
    printf("username: ");
    scanf("%s", username);
    
    if (strcmp(username, "quit") == 0){
        exit(0);
    } 
    printf("password: ");
    scanf("%s", password);

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    
    fp = fopen("./etc/passwd", "r");
    if (fp == NULL){
        printf("Password file does not exist");
        exit(EXIT_FAILURE);
    }
    
    int flag = 0;
    
    while ((read = getline(&line, &len, fp)) != -1) {
        char *token; 
        /* get the first token */
        token = strtok(line, ":");
        if (strcmp(token, username) == 0){
            /* walk through other tokens */
            while( token != NULL ) {
                if (strcmp(token,password) == 0){
                    flag = 1;
                    break;
                }
                token = strtok(NULL, ":");              
            }
        } 
    }
    fclose(fp);
    free(line);

    if(flag == 1){
        char * current = malloc(strlen(username)+1);
        strcpy(current, username);
        char proc_path[MAXLINE];
        char pid_str[MAXLINE];
        pid_t pid = getpid();
        strcpy(proc_path, "./proc/");
        sprintf(pid_str,"%d",pid);
        strcat(proc_path, pid_str);
    
        FILE * fp2;
        mkdir(proc_path, 0777);
        strcat(proc_path,"/status");
        fp2 = fopen(proc_path, "w+");
        fprintf(fp2,"Name: %s\n", "tsh");
        fprintf(fp2,"Pid: %d\n", pid);
        pid_t parent_pid = getppid();
        fprintf(fp2,"PPid: %d\n", parent_pid);
        pid_t group_pid = getpgid(pid);
        fprintf(fp2,"PGid: %d\n", group_pid);
        pid_t session_leader_id = getsid(pid);
        fprintf(fp2,"Sid: %d\n", session_leader_id);
        fprintf(fp2,"STAT: Fr\n");
        fprintf(fp2,"Username: %s\n", username);
        fclose(fp2);

        shell_pid = pid;
        
        //History 
        char history_path[MAXLINE];
        strcpy(history_path, "./home/");
        strcat(history_path, username);
        strcat(history_path, "/tsh_history");

        FILE * fp3;
        fp3 = fopen(history_path, "r");
        if (fp == NULL){
            printf("History file does not exist");
            exit(EXIT_FAILURE);
        }
        else{
            char * line = NULL;
            long int line_length = 0;
            long current_line = getline(&line, &line_length, fp3);
            int index = 0;
            while(current_line != -1 && index < MAXHISTORY){
                //printf("8");
                strcpy(history[index], line);
                free(line);
                line = NULL;
                index++;
                current_line = getline(&line, &line_length, fp3);
            }
            free(line);
            no_of_history = index;
            fclose(fp3);
        }

        fgets(line, sizeof(line), stdin); 
        return current;
    }
    else {
        printf("User Authentication failed. Please try again.\n");
        login();
    }
}    

void modify_history(char * command){
    if (no_of_history == MAXHISTORY) {
        for (int i = 1; i < MAXHISTORY; i++){
            strcpy(history[i-1], history[i]);
        }
        strcpy(history[no_of_history-1], command);
    }
    else{
        strcpy(history[no_of_history], command);
        no_of_history++;
    }
}

void save_history(){
    char path[MAXLINE] = "home/";
    strcat(path, username);
    strcat(path, "/tsh_history");
    FILE * fp8;
    fp8 = fopen(path, "w");
    if (fp8 == NULL){
        printf("Not able to access tsh_history file for user\n");
        exit(1);
    } 
    else{
        for (int i=0; i < MAXHISTORY; i++){
            fprintf(fp8, "%s", history[i]);
        }
    }
    fclose(fp8);
    return;
}
 /*
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{   
    
    char *argv[MAXARGS];    // Argument list execve()
    char buf[MAXLINE];      // Holds modified command line
    int bg;                 // Should the job run in bg or fg?
    pid_t pid;              // Process id
    sigset_t mask;          // Signal set to block certain signals
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL) {
        return;             // Ignore empty lines
    }
   
    if (!builtin_cmd(argv)) {
        // Blocking SIGCHILD
        sigemptyset(&mask);                    // initialize signal set
        sigaddset(&mask, SIGCHLD);             // addes SIGCHLD to the set
        sigprocmask(SIG_BLOCK, &mask, NULL);   // Adds the signals in set to blocked
        
        
        // Child
        if ((pid = fork()) < 0) {
            printf("Forking Error\n");
            exit(1);
        }
        else if (pid == 0) {
            setpgid(0, 0);                             // set child's group to a new process group (this is identical to the child's PID)
            sigprocmask(SIG_UNBLOCK, &mask, NULL);     // Unblocks SIGCHLD signal
            
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        }
        else {
            char proc_path[MAXLINE];
            char pid_str[MAXLINE];
            //pid_t pid = getpid();
            strcpy(proc_path, "./proc/");
            sprintf(pid_str,"%d",pid);
            strcat(proc_path, pid_str);
        
            FILE * fp4;
            mkdir(proc_path, 0777);
            strcat(proc_path,"/status");
            fp4 = fopen(proc_path, "w+");
            fprintf(fp4,"Name: %s\n", argv[0]);
            fprintf(fp4,"Pid: %d\n", pid);
            pid_t parent_pid = getppid();
            fprintf(fp4,"PPid: %d\n", parent_pid);
            pid_t group_pid = getpgid(pid);
            fprintf(fp4,"PGid: %d\n", group_pid);
            pid_t session_leader_id = getsid(pid);
            fprintf(fp4,"Sid: %d\n", shell_pid);
            if (!bg){
                fprintf(fp4,"STAT: FG\n");
            }
            else {
                fprintf(fp4,"STAT: BG\n");
            }  
            fprintf(fp4,"Username: %s\n", username);
            fclose(fp4);
        
        // Parent
        if (!bg) {  // Foreground
            addjob(jobs, pid, FG, cmdline);                             // Add this process to the job list
            sigprocmask(SIG_UNBLOCK, &mask, NULL);                 // Unblocks SIGCHLD signal
            waitfg(pid);                                                // Parent waits for foreground job to terminate
        } else {    // Background
            addjob(jobs, pid, BG, cmdline);                             // Add this process to the job list
            sigprocmask(SIG_UNBLOCK, &mask, NULL);                 // Unblocks SIGCHLD signal
            printf("[%d] (%d) %s", pid2jid(pid), (int)pid, cmdline);    // Print background process info
        }
    }
    delete_proc_file(pid);

    if (buf[0] != '!'){
        modify_history(buf);
        save_history();
        return;
    }
    return;
}
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

void delete_proc_file(int pid){
    char path[MAXLINE];
    char statuspath[MAXLINE];
    char pid_str[MAXLINE];
    strcpy(path, "./proc/");
    sprintf(pid_str,"%d",pid);
    strcat(path, pid_str);
    strcpy(statuspath, path);
    strcat(statuspath, "/status");
    remove(statuspath);
    rmdir(path);
}
/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    pid_t pid;
    char* command = argv[0];
    if (!strcmp(argv[0], "quit")) {
        pid = getpid();
        modify_history(argv[0]);
        for (int i=0; i< MAXJOBS; i++){
            //kill jobs
            if (jobs[i].pid != 0){
                int pid = jobs[i].pid;
                kill(pid, SIGTERM);
                delete_proc_file(pid);
            }
        }
        delete_proc_file(shell_pid);
        //free(username);
        exit(0);
    }

    else if (!strcmp(argv[0], "logout")){
        pid = getpid();
        modify_history(argv[0]);
        int jobs_running = 0;
        //Check jobs
        for (int i=0; i< MAXJOBS; i++){
        if (jobs[i].state == ST || BG){
            jobs_running = 1;
            printf("There are suspended jobs. Cannot logout.\n");
            return 1;
        }
        }
        if (jobs_running == 0) {
            for (int i=0; i< MAXJOBS; i++){
            //kill jobs
                if (jobs[i].pid != 0){
                    int pid = jobs[i].pid;
                    kill(pid, SIGTERM);
                    delete_proc_file(pid);
                }
            }
            delete_proc_file(shell_pid);
            //free(username);
            exit(0);
        }
    }

    else if (!strcmp(argv[0], "history")) {
        pid = getpid();
        modify_history("History\n");
        printf("User %s's history:\n", username);
        for (int i=1; i< no_of_history+1; i++){
            printf("%d: %s", i, history[i-1]);
        }

        strcpy(history[no_of_history], "");
        no_of_history--;
        return 1;
    }

    else if (command[0] == '!' && isdigit(command[1])) {
        pid = getpid();
        modify_history(argv[0]);
        char* c = command + 1;
        char b[strlen(c)+1];
        strcpy(b, c);
        //c[1] = '\0';
        //instruction_file[count] = atoi(c);
        int index = atoi(c);
        if (index < 1 || index > 10) {
            printf("History command only returns the last 10 commands.\nEnter a number between 1-10 (eg. !1)\n");
            return 1; 
        }
        else {
            if (!strcmp(history[index-1],"")) {
                printf("Unfortunately that entry does not exist, try a smaller index\n");
                return 1;
            }
            else {
                printf("%s", history[index-1]);
                return 1;
            }
        }
        return 1;
    }

    else if (!strcmp(argv[0], "jobs")) {
        pid = getpid();
        modify_history(argv[0]);
        listjobs(jobs);
        return 1;
    }
    else if (!strcmp(argv[0], "bg")) {
        pid = getpid();
        modify_history(argv[0]);
        do_bgfg(argv);
        return 1;
    }
    else if (!strcmp(argv[0], "fg")) {
        pid = getpid();
        modify_history(argv[0]);
        do_bgfg(argv);
        return 1;
    }

    else if (!strcmp(argv[0], "adduser")) {
        pid = getpid();
        modify_history(argv[0]);
        if (strcmp(username,"root")) {
            printf("Root priveleges needed to add new user.\n");
        }
        else{
            if (argv[1] == NULL || argv[2] == NULL) {
                printf("Either Username or password is missing, please reenter user credentials\n");
                return 1;
            }
            char * new_user = argv[1];
            FILE * fp5;
            char * line = NULL;
            size_t len = 0;
            ssize_t read;
            char buffer[MAXLINE];
            fp5 = fopen("./etc/passwd","r");
            //Assumes password file exists
            if (fp5 == NULL){
                printf("Password file does not exist");
                exit(EXIT_FAILURE);
            }
            
            int flag = 0;
            
            while ((read = getline(&line, &len, fp5)) != -1) {
                char *token; 
                /* get the first token */
                token = strtok(line, ":");
                if (strcmp(token, new_user) == 0){
                    /* walk through other tokens */
                    flag = 1;
                    break;
                }
                token = strtok(NULL, ":");                
            }
            free(line);
            fclose(fp5);
            
            if (flag == 1){
                printf("Duplicate username, cannot add user\n");
                return 1;
            }
            else if (flag == 0){
                char file_create [MAXLINE] = "home/";
                char file_location [MAXLINE] = "/";
                strcat(file_create, new_user);
                strcat(file_location, file_create);
                mkdir(file_create, 0777);
                strcat(file_create, "/tsh_history");

                FILE * fp6;
                fp6 = fopen(file_create, "a");
                if (fp6 == NULL) {
                    printf("Cannot create user history file");
                    fclose(fp6);
                    return 1;
                }
                fclose(fp6);
                
                FILE * fp7;
                fp7 = fopen("./etc/passwd", "a+");
                if (fp7 == NULL) {
                    printf("Cannot open password file");
                    fclose(fp7);
                    return 1;
                }
                else{
                    fseek(fp7, -1, SEEK_END);
                    char last = fgetc(fp7);
                    if (last != '\n') {
                        fprintf(fp7, "\n");
                    }
                    fprintf(fp7, "%s:%s:%s\n", new_user, argv[2], file_location);
                }
                fclose(fp7);
                return 1;
            }


        }
    }
    delete_proc_file(pid);
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    if (argv[1] == NULL) {                                      
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }
    
    if (!isdigit(argv[1][0]) && argv[1][0] != '%') {            // Checks if the second argument is valid
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    
    int is_job_id = (argv[1][0] == '%' ? 1 : 0);                // Checks if the second argument is refering to PID or JID
    struct job_t *givenjob;
    
    if (is_job_id) { 
        givenjob = getjobjid(jobs, atoi(&argv[1][1]));          // Get JID. pointer starts from the second character of second argument
        if (givenjob == NULL) {                                 // Checks if the given JID is alive
            printf("%s: No such job\n", argv[1]);
            return;
        }
    } else {       
        givenjob = getjobpid(jobs, (pid_t) atoi(argv[1]));      // Get PID with the second argument
        if (givenjob == NULL) {                                 // Checks if the given PID is there
            printf("(%d): No such process\n", atoi(argv[1]));
            return;
        }
    }
    
    
    if(strcmp(argv[0], "bg") == 0) {
        givenjob->state = BG;                                            // Change (FG > BG) or (ST ->  BG)
        printf("[%d] (%d) %s", givenjob->jid, givenjob->pid, givenjob->cmdline);
        kill(-givenjob->pid, SIGCONT);                              // Send SIGCONT signal to entire group of the given job
    } else {
        givenjob->state = FG;                                            // Change (BG -> FG) or (ST -> FG)
        kill(-givenjob->pid, SIGCONT);                              // Send SIGCONT signal to entire group of the given job
        waitfg(givenjob->pid);                                           // Wait for fg job to finish
    }
    

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{

    while (1) {
        if (pid != fgpid(jobs)) {
            if (verbose) printf("waitfg: Process (%d) no longer the fg process\n", (int) pid);
            break;
        } else {
            sleep(1);
        }
    }
    

    
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    // In sigchld_handler, use exactly one call to wait pid.
    // Must reap all available zombie children.
    if (verbose) printf("sigchld_handler: entering\n");

    pid_t pid;
    int status;
    int jobid;
    
    
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {    // Reap a zombie child
        jobid = pid2jid(pid);
        
        // Now checking the exit status of a reaped child
        
        // WIFEXITED returns true if the child terminated normally
        if (WIFEXITED(status)) {
            deletejob(jobs, pid); // Delete the child from the job list
            if (verbose) printf("sigchld_handler: Job [%d] (%d) deleted\n", jobid, (int) pid);
            if (verbose) printf("sigchld_handler: Job [%d] (%d) terminates OK (status %d)\n", jobid, (int) pid, WEXITSTATUS(status));
        }
        
        // WIFSIGNALED returns true if the child process terminated because of a signal that was not caught
        // For example, SIGINT default behavior is terminate
        else if (WIFSIGNALED(status)) {
            deletejob(jobs,pid);
            if (verbose) printf("sigchld_handler: Job [%d] (%d) deleted\n", jobid, (int) pid);
            printf("Job [%d] (%d) terminated by signal %d\n", jobid, (int) pid, WTERMSIG(status));
        }
        
        // WIFSTOPPED returns true if the child that cause the return is currently stopped.
        else if (WIFSTOPPED(status)) {     /*checks if child process that caused return is currently stopped */
            getjobpid(jobs, pid)->state = ST; // Change job status to ST (stopped)
            printf("Job [%d] (%d) stopped by signal %d\n", jobid, (int) pid, WSTOPSIG(status));
        }
        
    }
    
    if (verbose) printf("sigchld_handler: exiting\n");
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{

    if (verbose) printf("sigint_handler: entering\n");
    
    pid_t pid = fgpid(jobs);
    
    if (pid != 0) {
        // Sends SIGINT to every process in the same process group with pid
        kill(-pid, sig); // signals to the entire foreground process group
        if (verbose) printf("sigint_handler: Job [%d] and its entire foreground jobs with same process group are killed\n", (int)pid);
    }
    
    if (verbose) printf("sigint_handler: exiting\n");
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    
    pid_t pid = fgpid(jobs);
    
    if (pid != 0) {
        // Sends SIGTSTP to every process in the same process group with pid
        kill(-pid, sig); // signals to the entire foreground process group
        if (verbose) printf("sigtstp_handler: Job [%d] and its entire foreground jobs with same process group are killed\n", (int)pid);
    }
    
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}


