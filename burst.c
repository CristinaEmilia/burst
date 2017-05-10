/*
  burst.c
  splits a file into 500 lines segments
*/
#include "burst.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

void writeBuffer(char * file,char * buffer, int len)
{
    int writefd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if(writefd <= 0)
    {
        fprintf(stderr,"ERROR: unable to open %s for writing",file);
        return;
    }
    write(writefd,buffer,len);
    close(writefd);
}

int burst(char* fileName)
{
    int readfd = open(fileName, O_RDONLY);
    if(readfd <= 0)
    {
        fprintf(stderr,"ERROR: unable to open file %s",fileName);
        return -1;
    }


    int fileCount = 0;
    int lineCount = 0;
    char writeFile[512];
   
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
            lineBufferLen = lineBufferLen + BLOCK; 
            lineBuffer = (char*) realloc(lineBuffer,lineBufferLen);            
        }
        //copy the bytes read to the current readBufferfer
        memcpy(lineBuffer+lineBufferIndex,readBuffer,bytesRead);
        lineBufferIndex += bytesRead;
   
        int i;
        for (i = 0; i < bytesRead; i++)
        {
            if (readBuffer[i] == '\n' && ++lineCount >= LINE_COUNT)
            {
                lineCount = 0;
                int pid = fork();
                if(pid < 0)
                {
                    fprintf(stderr, "ERROR: unable to fork\n");
                    return 1;
                }

                snprintf(writeFile, 512, "%s.%d", fileName, ++fileCount);
                if(pid > 0) {  
                    //parent
                   
                    //set the line buffer index to 0, but keep using same line buffer
                    memcpy(lineBuffer,lineBuffer + lineBufferIndex - BLOCK + i + 1, BLOCK - i - 1);
                    lineBufferIndex = BLOCK - i + 1;
                } else { 
                    //child
                    writeBuffer(writeFile,lineBuffer, lineBufferIndex - BLOCK + i + 1);
                    return 0;
                }
            }
        }
    }

    if(lineBufferIndex > 0) {
        snprintf(writeFile, 512, "%s.%d", fileName, ++fileCount);
        if(fork() == 0)
        {
            writeBuffer(writeFile,lineBuffer,lineBufferIndex);
            return 0;
        }
    }

    close(readfd);

    return 0;
}

int main(int argc, char *argv[])
{
	if(argc < 1)
	{
		fprintf(stderr,"ERROR: Please provide file name. Example: burst file.txt");
		return -1;
	}

	return burst(argv[0]);
}
