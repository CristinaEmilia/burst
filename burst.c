/*
  burst.c
  splits a file into 500 lines segments
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BLOCK 2048




int main(int argc, char *argv[])
{
    int readfd = STDIN_FILENO;

    // open a file
    if (argc > 1)
    {
        readfd = open(argv[1], O_RDONLY);
    }

    int fileCount = 0;
    int lineCount = 0;
    char fileName[512];
   
    // read the opened file
    char readBuffer[BLOCK];
    int lineBufferLen = BLOCK;
    int lineBufferIndex = 0;
    char* lineBuffer = (char*)malloc(BLOCK);
   

    int bytesRead = 0;
    while ((bytesRead = read(readfd, readBuffer, BLOCK)) > 0)
    {
        if(lineBufferIndex + bytesRead > lineBufferLen)
        { 
            lineBuffer = (char*) realloc(lineBuffer,lineBufferLen + BLOCK);
            lineBufferLen = lineBufferLen + BLOCK; 
        }

        //copy the bytes read to the current readBufferfer
        memcpy(lineBuffer+lineBufferIndex,readBuffer,bytesRead);
        lineBufferIndex += bytesRead;

        int i;
        for (i = 0; i < bytesRead; i++)
        {
            if (readBuffer[i] == '\n')
            {
                int pid = fork();
                if(pid < 0)
                {
                    fprintf(stderr, "ERROR: unable to fork\n");
                    return 1;
                }

                snprintf(fileName, 512, "%s.%d", argv[1], ++fileCount);
                int lineBufferOffset = (BLOCK - i);
                if(pid > 0) {  
                    //parent
                   
                    //set the line buffer index to 0, but keep using same line buffer
                    memcpy(lineBuffer,lineBuffer + lineBufferIndex - lineBufferOffset, i);
                    lineBufferIndex = i;
                } else { 
                    //child
                    int writefd = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                    write(writefd,lineBuffer, lineBufferIndex - lineBufferOffset);
                    close(writefd);
                }
              
            }
        }
        
    }

    close(readfd);

    return 0;
}