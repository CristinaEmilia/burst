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

//borrowed from http://stackoverflow.com/a/2736841
char *remove_ext (char* mystr, char dot, char sep) {
    char *retstr, *lastdot, *lastsep;

    // Error checks and allocate string.

    if (mystr == NULL)
        return NULL;
    if ((retstr = malloc (strlen (mystr) + 1)) == NULL)
        return NULL;

    // Make a copy and find the relevant characters.

    strcpy (retstr, mystr);
    lastdot = strrchr (retstr, dot);
    lastsep = (sep == 0) ? NULL : strrchr (retstr, sep);

    // If it has an extension separator.

    if (lastdot != NULL) {
        // and it's before the extenstion separator.

        if (lastsep != NULL) {
            if (lastsep < lastdot) {
                // then remove it.

                *lastdot = '\0';
            }
        } else {
            // Has extension separator with no path separator.

            *lastdot = '\0';
        }
    }

    // Return the modified string.

    return retstr;
}

char *get_filename_ext(char *filename) {
    char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}


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
        {    
            data->lineCount += 1;
        }
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
                if(i < data->bytesRead - 1) {
                    memcpy(data->lineBuffer,data->lineBuffer + data->lineBufferIndex - data->bytesRead + i + 1, data->bytesRead - i - 1);
                    data->lineBufferIndex = data->bytesRead - i + 1;
                } else {
                    data->lineBufferIndex = 0;
                }
            } else { 
                //child

                writeBuffer(writeFile,data->lineBuffer, data->lineBufferIndex - data->bytesRead + i + 1);
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

int try_compressed(char * fileName, int raw)
{
    char * ext = get_filename_ext(fileName);
    if(strcmp(ext,"gz") != 0 && strcmp(ext,"bz2") != 0 && strcmp(ext,"tar") != 0 && strcmp(ext, "cpio") != 0 && strcmp(ext,"zip") != 0)
        return FALSE;
  
    int res;
    // setup the archive object
    struct archive* a = archive_read_new();
    archive_read_support_filter_all(a);
    if(raw)
        archive_read_support_format_raw(a);
    else
        archive_read_support_format_all(a);

    printf("testing archive\n");

    // actually open the archive file
    res = archive_read_open_filename(a, fileName, BLOCK);
    if(res != ARCHIVE_OK)
        return FALSE;

    // go to the first header
    struct archive_entry* entry;
    while((res = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
    {
        struct burst_data data;
        const char * entryName = archive_entry_pathname(entry);
        printf("archive_entry %s is ok.\n",entryName);
        if(strcmp(entryName,"data") != 0)
            data.fileName = (char*)entryName;
        else
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
        printf("decompressed\n");
    }
    
    archive_read_close(a);
    return TRUE;
}

int main(int argc, char *argv[])
{
	if(argc < 1)
	{
		fprintf(stderr,"ERROR: Please provide file name. Example: burst file.txt");
		return -1;
	}

    //attempt reading an archive
    if(try_compressed(argv[1],FALSE))
        return TRUE;

    //attempt reading compressed
    if(try_compressed(argv[1],TRUE))
        return TRUE;

    printf("attempting burst\n");
	return burst(argv[1]);
}
