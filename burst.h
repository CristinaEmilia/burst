#ifndef INCLUDED_BURST_H
#define INCLUDED_BURST_H

#define BLOCK 2048
#define LINE_COUNT 500


struct burst_data {
    char *fileName;
    char readBuffer[BLOCK];
    int fileCount;
    int lineCount;
    int bytesRead;
    int lineBufferLen;
    int lineBufferIndex;
    char * lineBuffer;
    int eof;
};

/* splits a given file into 500 line segments */
int burst(char* file);

#endif