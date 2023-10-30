#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#define DEFAULT_FREQ 1
#define DEFAULT_LOG "/tmp/watchdir.log"
#define DEFAULT_DIR "."

struct dirent **namelist1;
char * log;
char * dir;



void print_help(char * program_name)
{
    fprintf(stderr, "Usage: %s [-n SECONDS] [-l LOG] [DIR]\n", program_name);
    fprintf(stderr, "\tSECONDS Refresh rate in [1..60] seconds [default: 1].\n");
    fprintf(stderr, "\tLOG     Log file.\n");
    fprintf(stderr, "\tDIR     Directory name [default: '.'].\n\n");
}

int nodei_sort(const struct dirent **d1, const struct dirent **d2) {
    const long unsigned int a = (*d1)->d_ino;
    const long unsigned int b = (*d2)->d_ino;
    return(a - b);
}

void check_dir() {
    struct dirent **namelist2;
    int num_dir;
    FILE * log_file;
    if ((log_file = fopen(log, "a")) == NULL) {
        fprintf(stderr, "ERROR");
        exit(EXIT_FAILURE);
    }

    if ((num_dir = scandir(dir, &namelist2, NULL, nodei_sort)) == -1) {
        perror("scandir");
        exit(EXIT_FAILURE);
    }
    int i, j;
    j = i = 0;
    while(j < num_dir) {
        if (strncmp(namelist2[i]->d_name, ".", 1) == 0 || strncmp(namelist2[i]->d_name, "..", 2) == 0){
            continue;
        }

        if (namelist1[i]->d_ino == namelist2[i]->d_ino) {
            //mismo archivo
        }else if(namelist1[i]->d_ino < namelist2[i]->d_ino) {
            //Se ha borrado
            fprintf(log_file, "Deletion: %s\n", namelist1[i]->d_name);
        }else {
            //Se ha creado
            fprintf(log_file, "Creation: %s\n", namelist2[i]->d_name);
        }
    }
    namelist1 = namelist2;
    for (i = 0; i < num_dir; i++) {
        free(namelist2[i]);
    }
    free(namelist2);
    fclose(log_file);
}

void handler(int signo) {    
    if (signo == SIGALRM) {
        check_dir();
    }
}



int main(int argc, char *argv[])
{

    struct dirent **namelist2;
    int dir_flag = 0;
    int opt;
    int num_dir;
    int freq = DEFAULT_FREQ;
    log = DEFAULT_LOG;
    dir = DEFAULT_DIR;
    FILE * log_file;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR: REGEX vacía\n");
        exit(EXIT_FAILURE);
    }
    optind = 1;
    while (optind < argc)
    {
        if ((opt = getopt(argc, argv, "n:l:h")) != -1)
        {
            switch (opt)
            {
            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
            case 'n':
                freq = atoi(argv[optind - 1]);
                break;
            case 'l':
                log = argv[optind - 1];
                break;
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
                break;
            }
        }
        else
        {
            if (dir_flag != 0) {
                fprintf(stderr, "ERROR: './watchdir' does not support more than one directory.\n");
                print_help(argv[0]);
                exit(EXIT_FAILURE);

            }
            dir = argv[optind];
            optind++;
            dir_flag++;
        }
    }

    if (freq < 1 || freq > 60) {
        fprintf(stderr, "ERROR: SECONDS must be a value in [1..60].\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((log_file = fopen(log, "w")) == NULL) {
        fprintf(stderr, "ERROR");
        exit(EXIT_FAILURE);
    }

    if ((num_dir = scandir(dir, &namelist2, NULL, nodei_sort)) == -1) {
        fprintf(stderr, "ERROR: 'nodir' is not a directory.\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }
    int i;
    for (i = 0; i < num_dir; i++) {
        if (strncmp(namelist2[i]->d_name, ".", 1) == 0 || strncmp(namelist2[i]->d_name, "..", 2) == 0){
            continue;
        }
        fprintf(log_file, "Creation: %s\n", namelist2[i]->d_name);
    }
    namelist1 = namelist2;
    for (i = 0; i < num_dir; i++) {
        free(namelist2[i]);
    }
    free(namelist2);
    fclose(log_file);

    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = freq;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = freq;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1 ) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }

    while(1) {

    }
    exit(EXIT_SUCCESS);

}