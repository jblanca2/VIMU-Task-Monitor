/* This is the only file you should update and submit. */

/* Fill in your Name and GNumber in the following two comment fields
 * Name: Jan Michael Blanca
 * GNumber: G01213914
 */

#include <sys/wait.h>
#include "taskmntr.h"
#include "parse.h"
#include "util.h"

/* Constants */
#define DEBUG 1

/* 
// uncomment if you want to use any of these:

#define NUM_PATHS 2
#define NUM_INSTRUCTIONS 10

static const char *task_path[] = { "./", "/usr/bin/", NULL };
static const char *instructions[] = { "quit", "help", "list", "purge", "exec", "bg", "kill", "suspend", "resume", "pipe", "history", NULL};

*/

struct task
{
    int index; //index, position of task
    char *command; //command line
    int state;
    int exit; //exit state
    pid_t pid; 
    char **userLine; //argument line
    struct task *next;
};

struct task *hist = NULL;

struct task *history = NULL;

void take(char *cmd, char **args)
{
    struct task *temp = (struct task *) malloc(sizeof(struct task));

    temp->command = cmd;
    temp->userLine = args;

    temp->state = LOG_STATE_READY;

    temp->pid = 0;

    temp->exit = 0;

    temp->next = NULL;

    int num = 0;
    
    if(hist == NULL)
    {
        temp->index = 0;    

        hist = temp;

        return;
    }
    else
    {
        struct task *timp = hist;

        while(timp->next != NULL)
        {
            timp = timp->next;

            num++;
        }

        temp->index = num + 1;

        timp->next = temp;

        return;
    }
}

int count_hist(struct task *list)
{

    int count = 0;

    while(list != NULL)
    {
        count++;

        list = list->next;
    }

    return count;
}

void history_none(struct task *history)
{
    struct task *temp = history;


    log_history_info(count_hist(temp));

    while(temp != NULL)
    {
        log_history_commands(temp->index, temp->command);

        temp = temp->next;
    }
    
    return;
}

void histo_num(int num)
{
    //check for number in history
    struct task *temp = hist;

    while(temp != NULL)
    {
        if(temp->index == num)
        {
            log_history_exec(temp->command);
            return;
        }
    }

    //if it reaches here, history is at the end and there is no number in list 
    log_history_error(num);

    return;
}

//list built-in instruction
void list_task()
{
    struct task *temp = hist;

    log_num_tasks(count_hist(temp));

    while(temp != NULL)
    {
        log_task_info(temp->index, temp->state, temp->exit, temp->pid, temp->command);

        temp = temp->next;
    }

    return;
    
}

//delete built-in 
void delete(int index)
{   

    if(hist == NULL)//if there is nothing in the list, there is nothing to delete
    {
        log_task_num_error(index);
        return;
    }

    struct task *temp = hist;

    struct task *back = NULL;

    while(temp != NULL && temp->index != index)
    {
        back = temp;

        temp = temp->next;

    }

    if(temp == NULL) //means it reached end of list 
    {
        log_task_num_error(index);
    }

    if(temp->state ==  LOG_STATE_RUNNING || temp->state ==  LOG_STATE_SUSPENDED)
    {
        log_status_error(index, temp->state);
    }

    if(back == NULL)
    {
        hist = temp->next;
    }
    else
    {
        back->next = temp->next;
    }

    free(temp);

    log_purge(index);

    return;
}



void execute(int num, char *in, char *out)
{
    if(hist == NULL) //if list is empty
    {
        log_task_num_error(num);
        return;
    }

    struct task *temp = hist;

    while(temp != NULL && temp->index != num)
    {
        temp = temp->next;
    }

    if(temp->state == LOG_STATE_RUNNING || temp->state == LOG_STATE_SUSPENDED)
    {
        log_status_error(num, temp->state);
        return;
    }

    

    pid_t child = fork();

    if(child == 0)
    {
        if(in != NULL)
        {
            int fd = open(in, O_RDONLY);

            if(fd < 0)
            {
                log_file_error(num, in);
                exit(1);
            }

            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        if(out != NULL)
        {
            int fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if(fd < 0)
            {
                log_file_error(num, out);
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        log_status_change(num, temp->pid, LOG_FG, temp->command, LOG_START);

    }
    else
    {
        log_status_change(num, temp->pid, LOG_FG, temp->command, LOG_START);
    }

    log_exec_error(temp->command);
}

//background process
void background(int num, char *in, char *out)
{
    if(hist == NULL) //if list is empty
    {
        log_task_num_error(num);
        return;
    }

    struct task *temp = hist;

    while(temp != NULL && temp->index != num)
    {
        temp = temp->next;
    }

    if(temp->state == LOG_STATE_RUNNING || temp->state == LOG_STATE_SUSPENDED)
    {
        log_status_error(num, temp->state);
        return;
    }

    

    pid_t child = fork();

    if(child == 0)
    {
        if(in != NULL)
        {
            int fd = open(in, O_RDONLY);

            if(fd < 0)
            {
                log_file_error(num, in);
                exit(1);
            }

            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        if(out != NULL)
        {
            int fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

            if(fd < 0)
            {
                log_file_error(num, out);
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        log_status_change(num, temp->pid, LOG_BG, temp->command, LOG_START);

    }
    else
    {
        log_status_change(num, temp->pid, LOG_FG, temp->command, LOG_START);
    }

    log_exec_error(temp->command);
}

void piping(int num1, int num2)
{
    if(num1 == num2)
    {
        log_pipe_error(num1);
    }

    struct task *temp1 = hist;

    struct task *temp2 = hist;

    while(temp1->index != num1)
    {
        temp1 = temp1->next;
    }

    while(temp2->index != num2)
    {
        temp2 = temp2->next;
    }

    if(temp1 == NULL || temp2 == NULL || temp1->state == LOG_STATE_RUNNING || temp1->state == LOG_STATE_SUSPENDED || temp2->state == LOG_STATE_RUNNING || temp2->state == LOG_STATE_SUSPENDED)
    {
        return;
    }

    int pipe_fds[2] = {num1, num2};

    int succ = pipe(pipe_fds);

    int in = pipe_fds[1];

    int out = pipe_fds[0];

    pid_t child = fork();

    execute(num1, NULL, NULL);

    background(num2, NULL, NULL);

    if(succ < 0) //piping failed
    {   
        log_file_error(num1, LOG_FILE_PIPE);
        return;
    }

    if(child == 0)
    {
        close(in);
        dup2(out, STDIN_FILENO);
        execl("usr/bin/sort", "sort", NULL);
        log_pipe(num1, num2);
        exit(1);
    }
    else
    {
        close(out);
        dup2(in, STDOUT_FILENO);
        log_pipe(num1, num2);
    }

}

//kill instruction
void killer(int num)
{
    if(hist == NULL)
    {
        log_task_num_error(num);
    }

    struct task *temp = hist;

    while(temp != NULL)
    {
        temp = temp->next;
    }

    if(temp == NULL)
    {
        log_task_num_error(num);
    }

    if(temp->state == LOG_STATE_READY || temp->state == LOG_STATE_FINISHED || temp->state == LOG_STATE_KILLED)
    {
        log_status_error(num, temp->state);
    }

    log_sig_sent(LOG_CMD_KILL, num, temp->pid);
    kill(temp->pid, SIGINT);
    return;
}

//suspend instruction
void suspending_process(int num)
{
    if(hist == NULL)
    {
        log_task_num_error(num);
    }

    struct task *temp = hist;

    while(temp != NULL)
    {
        temp = temp->next;
    }

    if(temp == NULL)
    {
        log_task_num_error(num);
    }

     if(temp->state == LOG_STATE_READY || temp->state == LOG_STATE_FINISHED || temp->state == LOG_STATE_KILLED)
    {
        log_status_error(num, temp->state);
    }

    log_sig_sent(LOG_CMD_SUSPEND, num, temp->pid);
    kill(temp->pid, SIGTSTP);
    return;
}

//resume built-in instruction
void resuming(int num)
{
    if(hist == NULL)
    {
        log_task_num_error(num);
    }

    struct task *temp = hist;

    while(temp != NULL)
    {
        temp = temp->next;
    }

    if(temp == NULL)
    {
        log_task_num_error(num);
    }

     if(temp->state == LOG_STATE_READY || temp->state == LOG_STATE_FINISHED || temp->state == LOG_STATE_KILLED)
    {
        log_status_error(num, temp->state);
    }

    log_sig_sent(LOG_CMD_RESUME, num, temp->pid);
    kill(temp->pid, SIGCONT);
    return;
}

void c_handler(int sig)
{
    if(sig == SIGINT)
    {
        log_ctrl_c();
    }
}

void z_handler(int sig)
{
    if(sig == SIGTSTP)
    {
        log_ctrl_z();
    }
}

/* The entry of your task controller program */
int main() {
    char cmdline[MAXLINE];        /* Command line */
    char *cmd = NULL;

    /* Intial Prompt and Welcome */
    log_intro();
    log_help();

    /* Shell looping here to accept user command and execute */
    while (1) {
        char *argv[MAXARGS+1];        /* Argument list */
        Instruction inst;           /* Instruction structure: check parse.h */

        /* Print prompt */
        log_prompt();

        /* Read a line */
        // note: fgets will keep the ending '\n'
	errno = 0;
        if (fgets(cmdline, MAXLINE, stdin) == NULL) {
            if (errno == EINTR) {
                continue;
            }
            exit(-1);
        }

        if (feof(stdin)) {  /* ctrl-d will exit text processor */
          exit(0);
        }

        /* Parse command line */
        if (strlen(cmdline)==1)   /* empty cmd line will be ignored */
          continue;     

        cmdline[strlen(cmdline) - 1] = '\0';        /* remove trailing '\n' */

        cmd = malloc(strlen(cmdline) + 1);          /* duplicate the command line */
        snprintf(cmd, strlen(cmdline) + 1, "%s", cmdline);

        /* Bail if command is only whitespace */
        if(!is_whitespace(cmd)) {
            initialize_command(&inst, argv);    /* initialize arg lists and instruction */
            parse(cmd, &inst, argv);            /* call provided parse() */

            if (DEBUG) {  /* display parse result, redefine DEBUG to turn it off */
                debug_print_parse(cmd, &inst, argv, "main (after parse)");
	    }

            /* After parsing: your code to continue from here */
            /*================================================*/

            struct sigaction sa = {0};

            sa.sa_handler = c_handler;

            sigaction(SIGINT, &sa, NULL);

            struct sigaction ba = {0};

            ba.sa_handler = z_handler;

            sigaction(SIGTSTP, &ba, NULL);

            if(strcmp("help", argv[0]) == 0)
            {
                log_help();
            }
            else if(strcmp("quit", argv[0]) == 0)
            {
                log_quit();
                exit(0);
            }
            else if(strncmp("history", inst.instruct, 8) == 0)
            {
        
                history_none(hist);
            
            }
            else if(strncmp("list", inst.instruct, 4) == 0)
            {
                list_task();
            }
            else if(strncmp("delete", inst.instruct, 7) == 0)
            {
                delete(inst.num);
            }
            else if(strncmp("exec", inst.instruct, 4) == 0)
            {
                execute(inst.num, inst.infile, inst.outfile);
            }
            else if(strncmp("bg", inst.instruct, 2) == 0)
            {
                background(inst.num, inst.infile, inst.outfile);
            }
            else if(strncmp("pipe", inst.instruct, 4) == 0)
            {
                piping(inst.num, inst.num2);
            }
            else if(strncmp("kill", inst.instruct, 4) == 0)
            {
                killer(inst.num);
            }
            else if(strncmp("suspend", inst.instruct, 7) == 0)
            {
                suspending_process(inst.num);
            }
            else if(strncmp("resume", inst.instruct, 6) == 0)
            {
                resuming(inst.num);
            }
            else
            {
                take(cmd, argv);
            }

        }  // end if(!is_whitespace(cmd))

	free(cmd);
	cmd = NULL;
        free_command(&inst, argv);
    }  // end while(1)

    return 0;
}  // end main()

