#ifndef INCLUDED_BURST_H
#define INCLUDED_BURST_H

#define BLOCK 2048
#define LINE_COUNT 500
/* splits a given file into 500 line segments */
int burst(char* file);

#endif