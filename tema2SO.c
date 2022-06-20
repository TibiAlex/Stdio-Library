#include "so_stdio.h"

#include <string.h> //memset,strcpm
#include <fcntl.h> //open
#include <unistd.h> //close, read, write, lseek, fork, pipe, execvp
#include <sys/wait.h>   //waitpid
#include <sys/types.h>  //waitpid

#define dim_buf 4096 //dimensiunea maxima a buffer-ului

//structura so_file
typedef struct _so_file {
char const *pathname;   //numele path-ului
char const *mode;   //modul de deschidere al fisierului
int file_descriptor;    //file descriptorul fisierului
char buffer[dim_buf];   //bufferul in care se vor citi si scrie date
int buffer_pointer; //locatia actuala in buffer
int read_size;  //dimensiunea ce a fost citita din disier in buffer
int error_code; //codul de eroare inregistrat
int eof;    //variabila ce rezine daca s-a ajuns la finalul fisierului
char last_operation;    //variabila ce retine ultima operatie
int pid;    //id-ul procesului parinte
} SO_FILE;

/*  
    Functie ce se ocupa cu deschiderea fisierului in modul cerut
    --si initializarea structurii so_file
    --flagurile folosite in open sunt: 
    --O_RDONLY(pt fisier din care doar se citeste)
    --O_WRONLY(pt fisier din care doar se scrie)
    --O_RDWR(pt fisier in care se citeste si se scrie)
    --O_CREAT(pt a crea fisierul in cazul in care nu exista)
    --O_TRUNC(pt a goli fiserul in cazul in care se gaseste ceva in el)
    --O_APPEND(pt a scrie in continuarea fisierului)
    --modul de deschidere al fisierului a fost selectat 6 pt read-write
    */
FUNC_DECL_PREFIX SO_FILE *so_fopen(const char *pathname, const char *mode) {
    SO_FILE *stream = malloc(sizeof(SO_FILE));
    //initializarea membrilor structurii
    memset(stream->buffer, 0, dim_buf);
    stream->buffer_pointer = 0;
    stream->read_size = dim_buf;
    stream->error_code = 0;
    stream->eof = 0;
    stream->last_operation = 'o';
    stream->pathname = pathname;
    stream->mode = mode;
    stream->pid = 0;

    if (strcmp(stream->mode, "r") == 0)
        stream->file_descriptor = open(stream->pathname, O_RDONLY, 0600);
    else if (strcmp(stream->mode, "r+") == 0)
        stream->file_descriptor = open(stream->pathname, O_RDWR, 0600);
    else if (strcmp(stream->mode, "w") == 0)
        stream->file_descriptor = open(stream->pathname, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    else if (strcmp(stream->mode, "w+") == 0)
        stream->file_descriptor = open(stream->pathname, O_RDWR | O_CREAT | O_TRUNC, 0600);
    else if (strcmp(stream->mode, "a") == 0)
        stream->file_descriptor = open(stream->pathname, O_WRONLY | O_CREAT | O_APPEND, 0600);
    else if (strcmp(stream->mode, "a+") == 0)
        stream->file_descriptor = open(stream->pathname, O_RDWR | O_CREAT | O_APPEND, 0600);
    else
        stream->file_descriptor = SO_EOF;

    if (stream->file_descriptor == SO_EOF) {
        free(stream);
        return NULL;
    }
    return stream;
}

/*
    Functie ce se ocupa cu inchiderea fisierului
    --functia verifica daca mai exista ceva in buffer si scrie restul in fisier
    --daca este nevoie apoi incearca sa inchida fisierul si elibereaza memoria
    --structurii
 */
FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream) {
    int flush_flag = so_fflush(stream);
    int close_flag = close(stream->file_descriptor);
    char c = stream->last_operation;

    if (flush_flag == SO_EOF || close_flag == SO_EOF)
        stream->error_code = SO_EOF;

    free(stream);

    if (c  == 'w')
        return flush_flag;
    return close_flag;
}

// functie ce returneaza file descriptorul strucurii
FUNC_DECL_PREFIX int so_fileno(SO_FILE *stream) {
    return stream->file_descriptor;
}

/*
    Functie ce verifica daca ultima operatie facuta este write, caz in care
    --scrie in fisier pana goleste bufferul
    --apoi se reinitializeaza bufferul cu 0 si returneaza 0 in caz de succes
 */
FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream) {
    if (stream->last_operation == 'w') {
        int count_written = 0;

        while (count_written < stream->buffer_pointer) {
            int written_size = write(stream->file_descriptor,
            stream->buffer + count_written,
            stream->buffer_pointer - count_written);

            if (written_size == SO_EOF) {
                stream->error_code = SO_EOF;
                return SO_EOF;
            }
            count_written += written_size;
        }
        memset(stream->buffer, 0, dim_buf);
        stream->buffer_pointer = 0;
        stream->read_size = dim_buf;
    }
    return 0;
}

/*
    Functie care muta cursorul
    --exista 2 cazuri, in zarul lui read se reinitializeaza fufferul
    --in cazul in care ultima operatie a fost write se goleste bufferul prin flush
    --apoi se apeleaza functia lseek care muta offsetul unde este nevoie
 */
FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence) {
    if (stream->last_operation == 'r') {
        memset(stream->buffer, 0, dim_buf);
        stream->buffer_pointer = 0;
        stream->read_size = dim_buf;
    }
    if (stream->last_operation == 'w') {
        int flag = so_fflush(stream);

        if (flag == SO_EOF) {
            stream->error_code = SO_EOF;
            return SO_EOF;
        }
    }
    int flag = lseek(stream->file_descriptor, offset, whence);

    if (flag == SO_EOF) {
        stream->error_code = SO_EOF;
        return SO_EOF;
    }
    return 0;
}

/*
    --functiw ce returneaza pozitia curenta a offsetului
    --daca ultima operatie a fost read se scade numarul de elemente citite
    --in buffer si se adauga pointerul
    --daca ultima operatie a fost write se aduna direct numarul pointerului
 */
FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream) {
    int c = lseek(stream->file_descriptor, 0, SEEK_CUR);

    if (c == SO_EOF) {
        stream->error_code = SO_EOF;
        return SO_EOF;
    } else if (stream->last_operation == 'o')
        return c;
    else if (stream->last_operation == 'r')
        return c - stream->read_size + stream->buffer_pointer;
    else if (stream->last_operation == 'w')
        return c + stream->buffer_pointer;
    return SO_EOF;
}

/*
    Functie ce se ocupa cu scrierea in fisier, aceasta apeleaza functia fgetc
    --de size * nmemb ori si de fiecare data salveaza caracterul citit in 
    --vectorul de char-uri pointer, functia returneaza cate din cele nmemb
    --elemente a citit
 */
FUNC_DECL_PREFIX size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    char *pointer = (char *)ptr;
    int s = 0;

    for (int i = 0; i < size * nmemb; i++) {
        int flag = so_fgetc(stream);

        if (flag == SO_EOF) {
            stream->error_code = SO_EOF;
            return s/size;
        }
        pointer[i] = flag;
        s++;
    }
    return s/size;
}

/*
    Functie ce se ocupa cu scrierea in fisier, aceasta apeleaza functia fputc
    --de size * nmemb ori si de fiecare data scrie caracterul din vectorul de
    --char-uri pointer si returneaza cate din cele nmemb elemente a scris
 */
FUNC_DECL_PREFIX size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream) {
    char *pointer = (char *)ptr;
    int s = 0;

    for (int i = 0; i < size * nmemb; i++) {
        int flag = so_fputc(pointer[s], stream);

        if (flag == SO_EOF) {
            stream->error_code = SO_EOF;
            return s/size;
        }
        s++;
    }
    return s/size;
}

/*
    Functie care citeste verifica daca bufferul este gol sau daca s-a citit tot ce
    --este deja in bufffer apoi citeste din fiser numarul maxim de caractere pe care
    --le poate citii, adica 4096, si continua sa citeasca caracter cu caracte, dupa
    --cum i se cere
 */
FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream) {
    stream->last_operation = 'r';
    int flag = 0;

    if (stream->buffer_pointer == 0 ||
    stream->buffer_pointer == stream->read_size) {
        int read_size = read(stream->file_descriptor,
                            stream->buffer,
                            stream->read_size);

        if (read_size <= 0) {
            stream->error_code = SO_EOF;
            flag = SO_EOF;

            if (read_size == 0)
                stream->eof = 1;
        }
        stream->buffer_pointer = 0;
        stream->read_size = read_size;
    }
    return flag == SO_EOF ? SO_EOF : (int)((unsigned char)(stream->buffer[stream->buffer_pointer++]));
}

/*
    Functie ce verifica daca bufferul este plin, daca este il goleste
    --daca nu este scrie urmatorul caracter in buffer
 */
FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream) {
    stream->last_operation = 'w';
    int flag = 0;

    if (stream->buffer_pointer == stream->read_size) {
        int flush = so_fflush(stream);

        if (flush == SO_EOF) {
            stream->error_code = SO_EOF;
            flag = SO_EOF;
        }
    }
    if (flag == 0)
        stream->buffer[stream->buffer_pointer++] = c;
    return flag == SO_EOF ? SO_EOF : (int)((unsigned char)(c));
}

//fucntie care returneaza daca s-a ajuns la capatul fisierului sau nu
FUNC_DECL_PREFIX int so_feof(SO_FILE *stream) {
    return stream->eof;
}

//functie ce returneaza daca s-a intalnit vreo eroare sau 0 daca nu
FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream) {
    return stream->error_code;
}

/*
    Functia are ca rol rularea unei comenzi si returnarea unei strcturi de tipul so_file
    --declar o structura si initializez bufferul
    Folosind functia pipe cream un pipe pentru comunicarea intre procesul copil si procesul
    --parinte
    Vectorul filedes conține după execuția funcției pipe 2 descriptori de fișier:
    --filedes[0], deschis pentru citire;
    --filedes[1], deschis pentru scriere;
    Folosim functia fork pentru a crea un nou proces si verificam daca a fost creat cu success
    --procesele isi inchid din pipe capatul de care nu mai au nevoie folosind functia close
    --procesul copil executa comanda folosind functia execvp
 */
FUNC_DECL_PREFIX SO_FILE *so_popen(const char *command, const char *type) {
    SO_FILE *stream = malloc(sizeof(SO_FILE));
    memset(stream->buffer, 0, dim_buf);
    stream->buffer_pointer = 0;
    stream->read_size = dim_buf;
    stream->error_code = 0;
    stream->eof = 0;
    stream->last_operation = 'o';

    int filedes[2];
    if (pipe(filedes) < 0) {
        free(stream);
        return NULL;
    }

    int newfd;
    stream->pid = fork();

    if(stream->pid == SO_EOF) {
        free(stream);
        return NULL;
    } else if (stream->pid == 0) {
        if(type[0] == 'w') {
            close(filedes[1]);
            newfd = 0;
            dup2(filedes[0], newfd);
        }
        if(type[0] == 'r') {
            close(filedes[0]);
            newfd = 1;
            dup2(filedes[1], newfd);
        }

        const char *argv[] = {"sh", "-c", command, NULL};
        execvp("sh", (char *const *) argv);
        exit(SO_EOF);
    } else if (stream->pid > 0) {
        if(type[0] == 'w') {
            close(filedes[0]);
            stream->file_descriptor = filedes[1];
        }
        if(type[0] == 'r') {
            close(filedes[1]);
            stream->file_descriptor = filedes[0];
        }
        return stream;
    }
    
    free(stream);
    return NULL;
}

/*
    Functie ce are ca scop asteptarea terminarii procesului inceput de popen si
    golirea structurii SO_FILE
    Se foloseste functia waitpid pentru asteptarea procesului copil si intoarcerea statusului
    acestuia
 */
FUNC_DECL_PREFIX int so_pclose(SO_FILE *stream) {
    int pid =  stream->pid;
    so_fclose(stream);

    int status;

    if(waitpid(pid, &status, 0) == -1)
        return SO_EOF;
    return status;
}
