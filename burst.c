/*
  burst.c
  splits a file into 500 lines segments
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK 2048


int main(int argc, char *argv[])
{
    int infd = STDIN_FILENO;

    // open a file
    if (argc > 1)
    {
        infd = open(argv[1], O_RDONLY);
    }

    int fcount = 0;
    int linecount = 0;
    char fname[512];
    snprintf(fname, 512, "%s.%d", argv[1], ++fcount);
    int writefd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    // read the opened file
    char buf[BLOCK];

    int bytesread = 0;
    while ((bytesread = read(infd, buf, BLOCK)) > 0)
    {
        int newLineIndex = 0;
		int i;
        for (i = 0; i < bytesread; i++)
        {
            if (buf[i] == '\n')
            {
                if (++linecount < 500)
                    continue;

                if(fork() >= 0)
                    continue;
                    
                linecount = 0;
                write(writefd, buf + newLineIndex, i);
                snprintf(fname, 512, "%s.%d", argv[1], ++fcount);
                close(writefd);
                writefd = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                newLineIndex = i;
                continue;
                
            }
        }

        write(writefd,buf+newLineIndex,bytesread - newLineIndex);
    }
    
    close(writefd);
    close(infd);

    return 0;
}