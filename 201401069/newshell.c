#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<stdlib.h>     
#include<unistd.h>
#include<errno.h> 
#include<fcntl.h>
#include<sys/utsname.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<sys/ptrace.h>
#include<sys/types.h>
#define MAX 1024
#define MAX_COMM 100

// Global declarations
char cwd[MAX];   // Current working directory
char *all[MAX];
int current_out = 4;
int current_in = 5;
int fd[4];

typedef struct proc{         //for bg processes
    pid_t pid;
    int status;
    char *arg[MAX_COMM];
    struct proc *next;
}proc;
proc *start;

void sig_handle(int sig)
{
    if(sig == 20)
    {
        // make it background
        printf("This\n");
        //exit(1);


    }
    else if(sig == 2)
    {
        printf("\nInstead of Ctrl-C type quit\n");
        print_prompt();
    }
    else if(sig == 3)
    {
        printf("\nType quit to exit\n");
        print_prompt();
    }
    signal(sig, sig_handle);
}

void print_prompt()
{
    struct utsname uname_ptr;
    uname(&uname_ptr);
    getcwd(cwd, sizeof(cwd));
    setbuf(stdout, NULL);       //disable buffering
    printf("<%s@%s:%s> ",uname_ptr.nodename, uname_ptr.sysname, cwd);
}

void scan_command(char *command)
{
    int bytes_read;
    int nbytes = MAX;
    bytes_read = getline(&command, &nbytes, stdin);
    bytes_read -= 1;
    command[bytes_read] = '\0';
}

void *parse(char *command, int time) 
{
    char *comm;   // Stores arg or command to be returned
    if(time ==0)
        comm = strtok(command, " ");
    else
        comm = strtok(NULL, " ");
    return comm;
}

void parse_semicolon(char *command)
{
    int i ;
    for (i=0; i < MAX; i++)
        all[i] = (char *) malloc(MAX_COMM * sizeof(char));
    i =0;
    all[i] = strtok(command, ";");
    while(1)
    {
        i += 1;
        all[i] = strtok(NULL, ";");
        if(all[i] == NULL)
            break;
    }
}

void cd(char *arg)
{
    if(arg == NULL)
    {
        printf("insufficient arguments\n");
    }
    else
    {
        int cond;
        cond = chdir(arg);
        if(cond == -1)
        {
            printf("wrong path\n");
        }
    }
}

void jobs()
{
    bg_struct_handle(0, NULL, 2);
}

void kjob(char *arg[])
{
    if (arg[1] == NULL || arg[2] == NULL)
    {
        printf("insufficient args\n");
        return ;
    }
    int no;
    int sig;
    no = atoi(arg[1]) - 1;
    sig = atoi(arg[2]);
    proc *iterate;
    iterate = start;
    while(iterate != NULL && no != 0)
    {
        iterate = iterate -> next;
        no -= 1;
    }
    if(iterate == NULL)
    {
        printf("wrong input !\n");
        return ;
    }
    else 
    {
        int a;
        a = kill(iterate -> pid, sig);
        if(a == -1)
        {
            printf("KILL ERROR !\n");
            return ;
        }
        else
            bg_struct_handle(iterate -> pid, NULL, 1);
    }
}

void fg(char *arg[])
{
    int i;
    if(arg[1] == NULL)
    {
        printf("Insufficient arguments \n");
        return ;
    }
    else
        i = atoi(arg[1]) - 1;

    proc *iterate;
    iterate = start;
    while(iterate != NULL && i != 0)
    {
        iterate = iterate -> next;
    }
    if(iterate == NULL)
    {
        printf("Wrong job number\n");
    }
    else
    {
        pid_t pid;
        pid = iterate -> pid;
        bg_struct_handle(iterate -> pid, NULL, 1);
        wait(&pid);
        return ;
    }
}

void overkill(){
    proc *iterate;
    iterate = start;
    while(iterate != NULL)
    {
        int a;
        a = kill(iterate -> pid, 9);
        if(a == -1)
        {
            printf("KILL ERROR in pid: %d\n", iterate -> pid);
        }            
        else
        {
            printf("KILLED pid: %d\n", iterate -> pid);
            bg_struct_handle(iterate -> pid, NULL, 1);
        }
        iterate = iterate -> next;
    }
}

void bg_signal_handle()
{
    int status;
    pid_t pid;
    pid = waitpid(-1, &status, WNOHANG);
    proc *iterate;
    iterate = start;
    while(iterate != NULL)
    {
        if(iterate -> pid == getpid())
        {
            bg_struct_handle(pid, NULL, 1);
        }
    }
}


void bg_struct_handle(pid_t pid, char *arg[], int type)
{
    proc *iterate, *new;
    if(type == 0)        // new node
    {
        if(start == NULL)
        {
            start = (proc *)malloc(sizeof(proc));
            start -> pid = pid;
            start -> status = 1;
            start -> next = NULL;
            int i = 0;
            while(arg[i] != NULL)
            {
                start -> arg[i] = malloc(MAX_COMM * sizeof(char));
                strcpy(start -> arg[i], arg[i]);
                i += 1;
            }
            start -> arg[i] = NULL;
        }
        else
        {
            new = (proc *)malloc(sizeof(proc));
            new -> pid = pid;
            new -> status = 1;
            new -> next = NULL;
            int i = 0;
            while(arg[i] != NULL)
            {
                new -> arg[i] = malloc(MAX_COMM * sizeof(char));
                strcpy(new -> arg[i], arg[i]);
                i += 1;
            }
            new -> arg[i] = NULL;
            iterate = start ;
            while(iterate -> next != NULL)
                iterate = iterate -> next;
            iterate -> next = new;
        }
    }
    else if(type == 1)    // delete node
    {
        int i = 0;
        proc *preiter = NULL;
        iterate = start;
        while(iterate != NULL && iterate -> pid != pid )
        {
            preiter = iterate;
            iterate = iterate -> next;
        }
        if(iterate == NULL)
        {
            printf("No Such Pid !\n");
            return ;
        }
        else if(iterate -> pid == pid)
        {
            if(preiter == NULL)
            {
                start = iterate -> next;
                free(iterate);
            }
            else
            {
                preiter -> next = iterate -> next;
                free(iterate);
            }
        }
    }
    else if(type == 2)    //iterate and print
    {
        int i = 1, a = 0;
        iterate = start;
        if (iterate == NULL)
        {
            printf("No Background jobs\n");
            return ;
        }
        while(iterate != NULL)
        {
            a = 0;
            setbuf(stdout, NULL);       //disable buffering
            printf("[%d] ",i);
            while(iterate -> arg[a] != NULL)
            {
                printf("%s ", iterate -> arg[a]);
                a += 1;
            }
            printf("[%d]\n", iterate -> pid);
            i += 1;
            iterate = iterate -> next;
        }
    }
    return ;
}

void bf_exec(char *arg[], int type)
{
    pid_t pid;
    if(type == 0)    // foreground
    {
        if((pid = fork()) < 0)
        {
            printf("*** ERROR: forking child process failed\n");
            return ;
        }
        else if(pid == 0)    //child
        {
            //signal(SIGTSTP, sig_handle);
            printf("Hello\n");
            printf("DEBUG : pgrp: %d, pid: %d term: %d STDIN_FILE: %d \n", getpgrp(), getpid(), tcgetpgrp(STDIN_FILENO), STDIN_FILENO);
            execvp(arg[0], arg);
        }
        else     //parent
        {
            pid_t c;
            c = wait(&pid);
            dup2(current_out, 1);
            dup2(current_in, 0);
        }
    }
    else            // background
    {
        signal(SIGCHLD, bg_signal_handle);
        if((pid = fork()) < 0)
        {
            printf("*** ERROR: forking child process failed\n");
            return ;
        }
        else if(pid == 0)    //child
        {
            execvp(arg[0], arg);
        }
        else     //parent
        {
            printf("DEBUG : pgrp: %d, pid: %d term: %d STDIN_FILE: %d \n", getpgrp(), getpid(), tcgetpgrp(STDIN_FILENO), STDIN_FILENO);
            bg_struct_handle(pid, arg, 0);
            pid = 0;
            //dup2(current_out, 1);
            //dup2(current_in, 0);
            return ;
        }

    }
}

void file_out(char *arg[], char *out_file, int type)
{
    int f;
    current_out = dup(1);         // For resetting the stdout
    if(type == 0)        //no_append
    {
        f = open(out_file, O_WRONLY | O_CREAT, 0777);
        dup2(f, 1);
        close(f);
        bf_exec(arg, 0);
    }
    else                 // append
    {
        f = open(out_file, O_WRONLY | O_CREAT | O_APPEND , 0777);
        dup2(f, 1);
        close(f);
        bf_exec(arg, 0);
    }
}

void file_in(char *arg[], char *in_file, char *out_file, int type)
{
    int in;
    in = open(in_file, O_RDONLY);
    current_in = dup(0);
    dup2(in, 0);
    close(in);
    if(type == 0)    // single in-redirecrtion
    {
        printf("Going to execute bf_exec\n");          //debug remove it
        bf_exec(arg, 0);
    }
    else if(type == 1)            // dual redirection  > 
    {
        file_out(arg, out_file, 0);
    }
    else                         // dual redirection >>
    {
        file_out(arg, out_file, 1);
    }
}

void pipe_exec(char *arg[], char *arg2[], char *arg3[], char *in_file, int type)      // not working
{
    pid_t childpid, cocpid, c;
    if(type == 0)            // one pipe without infile
    {
        if(pipe(fd) < 0)
            perror("pipe error \n");

        if((childpid = fork()) < 0)
            perror("Fork error\n");

        else if(childpid == 0)
        {
            dup2(0,0);
            dup2(fd[1], 1);
            close(fd[0]);
            execvp(arg[0], arg);
        }
        else
        {
            wait(NULL);
            close(fd[1]);
            if((cocpid = fork()) < 0)
                error("Fork error\n");
            else if(cocpid == 0)
            {
                dup2(fd[0], 0);
                close(fd[0]);
                execvp(arg2[0], arg2);
            }
            else
            {
                wait(NULL);
                close(fd[1]);
                return ;
            }
        }
    }
    else if(type == 1)       // one pipe with infile
    {
        printf("In one pipe with infile\n");          //debug remove
        if(pipe(fd) < 0)
            perror("pipe error \n");
        dup2(fd[1], 1);
        close(fd[0]);
        fprintf(stdout, "file_in complete\n");         //debug remove it 
        file_in(arg, in_file, NULL, 1);
        fprintf(stdout, "file_in complete\n");         //debug remove it 
        if(cocpid = fork() < 0)
            error("Fork error\n");
        else if(cocpid == 0)
        {
            dup2(fd[0], 0);
            close(fd[0]);
            execvp(arg2[0], arg2);
        }
        else
        {
            wait(NULL);
            close(fd[1]);
            return ;
        }
    }
    else if(type == 2)       //two pipes without infile 
    {
        if(pipe(fd) < 0)
            perror("Pipe error\n");
        if(pipe(fd+2) < 0)
            perror("pipe error\n");
        if((childpid = fork()) < 0)
            perror("fork error\n");
        if(childpid == 0)
        {
            dup2(0,0);
            dup2(fd[1], 1);
            close(fd[0]);
            execvp(arg[0], arg);
        }
        else
        {
            wait(NULL);
            close(fd[1]);
            if((cocpid = fork()) < 0)
                perror("Fork error\n");

            else if(cocpid == 0)
            {
                dup2(fd[0], 0);
                dup2(fd[3], 1);
                close(fd[0]);
                close(fd[3]);
                execvp(arg2[0], arg2);
            }
            else
            {
                wait(NULL);
                close(fd[0]);
                if((childpid = fork()) < 0)
                    perror("Fork error \n");
                else if(childpid == 0)
                {
                    dup2(fd[2], 0);
                    dup2(1,1);
                    close(fd[2]);
                    execvp(arg3[0], arg3);
                }
                else
                {
                    wait(NULL);
                    close(fd[3]);
                    return ;
                }
            }
        }
    }
}

void execute(char *command)
{
    int already = 0;
    char *arg[MAX_COMM];
    char *try;
    arg[0] = parse(command, 0);        //Root Command
    int t = 1;
    arg[t] = NULL;   

    if (strcmp(arg[0], "cd") == 0)     // command is cd
    {
        try = parse(command, 1);
        cd(try);
        already = 1;
        return ;
    }

    while(1)        // For subsequent arguments
    {
        try = parse(command, 1);
        if (try == NULL)
            break;

        else if (strcmp(try,">") == 0)  //Case of redirection to a file
        {
            try = parse(command, 1);   // to get the out file
            already = 1;
            file_out(arg, try, 0);        // 0 for no APPEND
        }

        else if(strcmp(try, ">>") == 0)   // Redirection append to a file
        {
            try = parse(command, 1);
            already = 1;
            file_out(arg, try, 1);        // 1 for APPEND to file
        }

        else if(strcmp(try, "<") == 0)    // stdin redirect from a file
        {
            already = 1;
            try = parse(command, 1);      // contains infile
            char *out_file = parse(command, 1);    // will contain outfile but first > or >> or NULL
            if (out_file != NULL)
            {
                if(strcmp(out_file, ">") == 0)   // both input and output file specified
                {
                    out_file = parse(command, 1);
                    if (out_file == NULL)
                    {
                        printf("Syntax error !!\n");
                        return ;
                    }
                    else
                        file_in(arg, try, out_file, 1);     // 1 mode for > dual redirection
                }
                else if(strcmp(out_file, ">>") == 0)      
                {
                    out_file = parse(command, 1);
                    if(out_file == NULL)
                    {
                        printf("Syntax error !!\n");
                        return ;
                    }
                    else
                        file_in(arg, try, out_file, 2);       // 2 mode for >>
                }
                else if(strcmp(out_file, "|") == 0)           // IO + pipe
                {
                    char *arg2[MAX];
                    int a2 = 0;
                    char *try2;
                    try2 = parse(command, 1);
                    if(try2 == NULL)
                    {
                        printf("Insufficient args for piping\n");
                        return ;
                    }
                    else
                    {
                        a2 += 1;
                        while((try2 = parse(command, 1)) != NULL)
                        {
                            arg2[a2] = try;
                            a2 += 1;
                            arg2[a2] = NULL;
                        }
                        // call for execution through pipes ... NOTDONE YET
                        pipe_exec(arg, arg2, NULL, try, 1);
                    }
                }
            }
            else 
            {
                file_in(arg, try, NULL, 0);        // 0 mode for single in-redirection
            }
        }

        else if(strcmp(try, "|") == 0)   // pipe to another command 
        {
            already = 1;
            char *arg2[MAX];       // for 2nd command
            char *arg3[MAX];             // for 3rd command
            int a2 = 0;            // counter for 2nd command and its argument
            arg2[a2] = parse(command, 1);
            if(arg2[a2] == NULL)
            {
                printf("Insufficient Arguments !!\n");
                return ;
            }
            else       // collection of the args of 2nd command
            {
                a2 += 1;
                while((try = parse(command, 1)) != NULL && strcmp(try, "|") != 0)
                {
                    arg2[a2] = try;
                    a2 += 1;
                    arg2[a2] = NULL;
                }
                if(try != NULL)
                {
                    if(strcmp(try, "|") == 0)        // ie. 3rd command also present
                    {
                        int a3 = 0;
                        arg3[a3] = parse(command, 1); 
                        if(arg3[a3] == NULL)
                        {
                            printf("Insufficient Arguments for 2nd pipe!!\n");
                            return ;
                        }
                        else                           // collection of args for 3rd command
                        {
                            a3 += 1;
                            while((try = parse(command, 1)) != NULL)
                            {
                                arg3[a3] = try;
                                a3 += 1;
                                arg3[a3] = NULL;
                            }
                        }
                    }
                    pipe_exec(arg, arg2, arg3, NULL, 2);
                }
                else                             // only one pipe
                {
                    pipe_exec(arg, arg2, NULL, NULL, 0);
                }
            }
            //pipe_exec(arg, arg2, arg3, NULL, 2);
        }
        else if(strcmp(try, "&") == 0)   // background process
        {
            already = 1;
            bf_exec(arg, 1);     // 1 for background
            return ;
        }

        else       //yet another argument
        {
            arg[t] = try;
            t += 1;
            arg[t] = NULL;
        }
    }
    if (already == 0)
    {
        if(strcmp(arg[0], "jobs") == 0)
            jobs();                   // call ud-function jobs
        else if(strcmp(arg[0], "kjob") == 0)
            kjob(arg);
        else if(strcmp(arg[0], "fg") == 0)
            fg(arg);
        else if(strcmp(arg[0], "overkill") == 0)
            overkill();
        else if(strcmp(arg[0], "quit") == 0)
        {
            printf("Doing clean up....\n");
            overkill();
            exit(0);
        }
        else
            bf_exec(arg, 0);     // 0 for foreground    
    }
}


int main()
{
    char *command, *root_command;
    int iter = 0;                          // for ; seperated commands
    int time = 0;
    command = (char *)malloc(MAX+1);
    chdir("/home/shaleen");
    while(1)
    {
        //        time = 0;
        iter = 0;
        signal(SIGINT, sig_handle);
        signal(SIGQUIT, sig_handle);
        signal(SIGCHLD, sig_handle);
        signal(SIGTSTP, sig_handle);
        print_prompt();  
        scan_command(command);             // Scan the full command
        parse_semicolon(command);          // Parse according to the ; and return all in all[]
        while(all[iter] != NULL)
        {
            time = 0;
            execute(all[iter]);
            iter += 1;
            //            time = 1;                           // FOR parsing the same command again
        }
    }
}
