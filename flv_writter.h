#ifndef _FLV_WRITTER_H_
#define _FLV_WRITTER_H_

#include <stdio.h>

typedef struct  
{
    FILE* file_handle_;
} FlvWritter;

void FlvWritter_Open(FlvWritter* flvWritter, const char* filename);

void FlvWritter_Close(FlvWritter* flvWritter);

void FlvWritter_WriteAACSequenceHeaderTag(FlvWritter* flvWritter,
    int sampleRate, int channel);

void FlvWritter_WriteAVCSequenceHeaderTag(FlvWritter* flvWritter,
    const char* spsBuf, int spsSize,
    const char* ppsBuf, int ppsSize);

void FlvWritter_WriteAACDataTag(FlvWritter* flvWritter, const char* dataBuf, 
    int dataBufLen, int timestamp);

void FlvWritter_WriteAVCDataTag(FlvWritter* flvWritter, const char* dataBuf, 
    int dataBufLen, int timestamp, int isKeyframe);

void FlvWritter_WriteAudioTag(FlvWritter* flvWritter, char* buf, 
    int bufLen, int timestamp);

void FlvWritter_WriteVideoTag(FlvWritter* flvWritter, char* buf, 
    int bufLen, int timestamp);

#endif // _FLV_WRITTER_H_
