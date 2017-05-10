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
#include <archive.h>
#include <archive_entry.h>

#define TRUE 1
#define FALSE 0




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

void processBurst(struct burst_data *data)
{
    if(data->eof) 
    {
        if(data->lineBufferIndex > 0) {
            char writeFile[512];
            data->fileCount+=1;
            snprintf(writeFile, 512, "%s.%d", data->fileName, data->fileCount);
            if(fork() == 0)
            {
                writeBuffer(writeFile,data->lineBuffer,data->lineBufferIndex);
                _exit(EXIT_SUCCESS);
            }
        }
        return;
    }
    

    if(data->lineBufferIndex + data->bytesRead > data->lineBufferLen)
    { 
        data->lineBufferLen = data->lineBufferLen + BLOCK; 
        data->lineBuffer = (char*) realloc(data->lineBuffer,data->lineBufferLen);            
    }
    //copy the bytes read to the current readBuffer
    memcpy(data->lineBuffer+data->lineBufferIndex,data->readBuffer,data->bytesRead);
    data->lineBufferIndex += data->bytesRead;

    int i;
    for (i = 0; i < data->bytesRead; i++)
    {
        if (data->readBuffer[i] == '\n')
            data->lineCount += 1;
        if(data->lineCount >= LINE_COUNT)
        {
            data->lineCount = 0;
            int pid = fork();
            if(pid < 0)
            {
                fprintf(stderr, "ERROR: unable to fork\n");
                _exit(EXIT_FAILURE);
            }
            char writeFile[512];
            data->fileCount += 1;
            snprintf(writeFile, 512, "%s.%d", data->fileName, data->fileCount);
            if(pid > 0) {  
                //parent
                
                //set the line buffer index to 0, but keep using same line buffer
                memcpy(data->lineBuffer,data->lineBuffer + data->lineBufferIndex - BLOCK + i + 1, BLOCK - i - 1);
                data->lineBufferIndex = BLOCK - i + 1;
            } else { 
                //child

                writeBuffer(writeFile,data->lineBuffer, data->lineBufferIndex - BLOCK + i + 1);
                _exit(EXIT_SUCCESS);
            }
        }
    }
}

int burst(char* fileName)
{
    int readfd = open(fileName, O_RDONLY);
    if(readfd <= 0)
    {
        fprintf(stderr,"ERROR: unable to open file %s",fileName);
        return -1;
    }

    struct burst_data data;
    data.fileName = fileName;
    data.eof = FALSE;
    data.fileCount = 0;
    data.lineCount = 0;

    // read the opened file
    data.lineBufferLen = BLOCK;
    data.lineBufferIndex = 0;
    data.lineBuffer = (char*)malloc(BLOCK);
    data.bytesRead = 0;

    while ((data.bytesRead = read(readfd, data.readBuffer, BLOCK)) > 0)
    {
        processBurst(&data);
    }

    data.eof = TRUE;

    processBurst(&data);
    close(readfd);

    return 0;
}

int try_compressed(char * fileName)
{
    int res;
    // setup the archive object
    struct archive* a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);

    printf("testing archive\n");

    // actually open the archive file
    res = archive_read_open_filename(a, fileName, 10240);
    if(res != ARCHIVE_OK)
        return FALSE;

    // go to the first header
    struct archive_entry* entry;
    res = archive_read_next_header(a, &entry);
    if(res != ARCHIVE_OK)
        return FALSE;

    printf("archive_entry is ok.\n");

    struct burst_data data;
    data.fileName = fileName;
    data.eof = FALSE;
    data.fileCount = 0;
    data.lineCount = 0;

    // read the opened file
    data.lineBufferLen = BLOCK;
    data.lineBufferIndex = 0;
    data.lineBuffer = (char*)malloc(BLOCK);
    data.bytesRead = 0;
    

    for (;;) 
    {
        int size = archive_read_data(a, data.readBuffer, BLOCK);
        if (size < 0)
        {
            fprintf(stderr,"ERROR: Unable to read archive data");
            _exit(EXIT_FAILURE);
        }

        // EOF
        if (size == 0)
            break;

        data.bytesRead = size;
        processBurst(&data);
    }

    data.eof = TRUE;
    processBurst(&data);

    // close it up
    archive_read_close(a);
    printf("decompressed\n");
    return TRUE;
}

int main(int argc, char *argv[])
{
	if(argc < 1)
	{
		fprintf(stderr,"ERROR: Please provide file name. Example: burst file.txt");
		return -1;
	}

    if(try_compressed(argv[1]))
        return TRUE;

    printf("attempting burst\n");
	return burst(argv[1]);
}
