#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>
#include <string.h>

#define DEFAULT_BUF_SIZE 1024
#define MAX_LINE_SIZE 4096
#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

int negation_flag = 0;

void print_help(char* program_name)
{
    fprintf(stderr, "Uso: %s -r REGEX [-s BUFSIZE] [-v] [-h]\n", program_name);
    fprintf(stderr, "\t-r REGEX Expresión regular.\n");
    fprintf(stderr, "\t-s BUFSIZE Tamaño de los buffers de lectura y escritura en bytes (por defecto, 1024).\n");
    fprintf(stderr, "\t-v Muestra las líneas que NO sean reconocidas por la expresión regular.\n\n");
    
}

int write_all(char *buf, ssize_t buf_size, unsigned writebuf_size) 
{
    int fd = STDOUT_FILENO;
    ssize_t remaining = buf_size;
    ssize_t num_written = 0;
    ssize_t total_written;

    char *buf_left = buf;
    for (size_t i = 0; i < buf_size; i += num_written) {
        remaining = remaining - num_written;
        size_t write_size = (remaining < writebuf_size) ? remaining : writebuf_size;
        num_written = write(fd, buf + i, write_size);  
    }
    return num_written == -1? -1 : buf_size;
}

int regex_comprobation(regex_t *regex, char * line, int pmatch_size, regmatch_t * pmatch, int flags) {
    if (negation_flag == 1) {
        if (regexec(regex, line, pmatch_size, pmatch, flags)) {
            return 1;
        } else {
            return 0;
        }
    } else {
        if (regexec(regex, line, pmatch_size, pmatch, flags)) {
            return 0;
        } else {
            return 1;
        }
    }
}

int checkRegex( char *buf, unsigned buf_size, regex_t *regex, char * line, unsigned writebuf_size) {
    
    ssize_t num_written;
    int line_counter = 0;
    int buf_position = 0;
    regmatch_t pmatch[1];
    regoff_t off, len;
    memset(line, 0, MAX_LINE_SIZE);

    int i;


    while (buf_position < buf_size ) {
        if (buf[buf_position] == '\n' || buf[buf_position] == '\0') {  
            if (line_counter >= MAX_LINE_SIZE) {
            fprintf(stderr, "ERROR: Línea demasiado larga\n");
            exit(EXIT_FAILURE);
            }
            line[line_counter] = '\0';            
            if (regex_comprobation(regex, line, 1, pmatch, 0)){
                line[line_counter] = '\n';
                num_written = write_all(line, line_counter+1, writebuf_size);
                if (num_written == -1)
                {
                    fprintf(stderr, "ERROR: write()\n");
                    exit(EXIT_FAILURE);
                }
                assert(num_written == line_counter + 1);
            }
            line_counter = 0;
        } else {
            if (line_counter >= MAX_LINE_SIZE) {
            fprintf(stderr, "ERROR: Línea demasiado larga\n");
            exit(EXIT_FAILURE);
            }
            line[line_counter] = buf[buf_position];
            line_counter++;
        } 
        buf_position++;
        
    }
    
    return line_counter;

}

void check_string(int fdin, int fdout, char *buf, unsigned buf_size, regex_t *regex)
{
    ssize_t num_read, num_written;
    int buff_init;
    char * buffer_restante;
    char * line;
    //Se actualiza la memoria que se reserva para el buffer restante, teniendo
    //en cuenta que en el peor de los casos con el tamaño mínimo de buffer se almacenarán dos lineas 
    if (buf_size < 4096 * 2) {
        buff_init = 4096 * 2;
    } else if (buf_size > 4096 * 2) {
        buff_init = buf_size;
    }
    unsigned writebuf_size = buf_size;
    if ((buffer_restante = (char *) malloc(buff_init * sizeof(char))) == NULL)
    {
        fprintf(stderr, "ERROR: malloc()\n");
        exit(EXIT_FAILURE);
    }
    if ((line = (char *) malloc(MAX_LINE_SIZE * sizeof(char))) == NULL)
    {
        fprintf(stderr, "ERROR: malloc()\n");
        exit(EXIT_FAILURE);
    }
    int buf_position;
    int restantes = 0;
    int i, j;
    j = 0;
    while ((num_read = read(fdin, buf, buf_size)) > 0)
    {
        for (i = 0; i < num_read; i++) {
            buffer_restante[j] = buf[i];
            j++;
        }

        restantes = checkRegex(buffer_restante, j, regex, line, writebuf_size);

        //Se introduce lo que queda por procesar al inicio del buffer
        if (restantes != 0) {
            int k = j - restantes;
            for (i = 0 ; i < restantes; i++) {
                buffer_restante[i] = buffer_restante[k];
                k++;
            }
            j = restantes;
        } else {
            j = 0;
        }
        

    }
    if (num_read == -1)
    {
        fprintf(stderr, "ERROR: read()\n");
        exit(EXIT_FAILURE);
    }

    //Si se ha quedado algo en el buffer de restantes(se ha llegado al final de la entrada y no hay
    //fin de cadena) se añade un salto de línea y se procesa
    if (j != 0) {
        buffer_restante[j] = '\n';
        checkRegex(buffer_restante, j+1, regex, line, writebuf_size);
    }
    free(line);
    free(buffer_restante);  
}



int main(int argc, char *argv[]) {

    int bufsize = DEFAULT_BUF_SIZE;
    int opt;
    const int fdin = STDIN_FILENO;
    const int fdout = STDOUT_FILENO;
    char *buf;
    optind = 1;
    int reg_pos;
    regex_t regex;
    char *regular_expression;

    if (argc < 2) {
        fprintf(stderr, "ERROR: REGEX vacía\n");
        exit(EXIT_FAILURE);
    }
    while ((opt = getopt(argc, argv, "r:hvs:")) != -1)
    {
        switch (opt)
        {
        case 'r':
            //Guardamos la última expresión regular para compilarla posteriormente
            regular_expression = argv[optind-1];
            break;
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        case 'v':
            //activamos un flag que hace que checkregex funcione al contrario
            negation_flag = 1;
            break;
        case 's':
            bufsize = atoi(argv[optind-1]);
            break;
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (bufsize < 1 || bufsize > 1048576) 
    {
        fprintf(stderr, "ERROR: BUFSIZE debe ser mayor que 0 y menor que o igual a 1 MB\n");
        exit(EXIT_FAILURE);
    } 
    if ((buf = (char *) malloc(bufsize * sizeof(char))) == NULL)
    {
        fprintf(stderr, "ERROR: malloc()\n");
        exit(EXIT_FAILURE);
    }
    if(regcomp(&regex, regular_expression, REG_NEWLINE | REG_EXTENDED) != 0) {
        fprintf(stderr, "ERROR: REGEX mal construida\n");
        exit(EXIT_FAILURE);
    };

    check_string(fdin, fdout, buf, bufsize, &regex);    
    free(buf);
    exit(EXIT_SUCCESS);
}
