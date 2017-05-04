#ifndef _FLV_READER_H_
#define _FLV_READER_H_

#include <stdio.h>

#define FLV_UI32(x) (int)(((x[0]) << 24) + ((x[1]) << 16) + ((x[2]) << 8) + (x[3]))
#define FLV_UI24(x) (int)(((x[0]) << 16) + ((x[1]) << 8) + (x[2]))
#define FLV_UI16(x) (int)(((x[0]) << 8) + (x[1]))
#define FLV_UI8(x) (int)((x))

#define FLV_AUDIODATA	8
#define FLV_VIDEODATA	9
#define FLV_SCRIPTDATAOBJECT	18

typedef struct 
{
    unsigned char signature[3];
    unsigned char version;
    unsigned char flags;
    unsigned char headersize[4];
} FlvFileHeader;

typedef struct 
{
    unsigned char type;
    unsigned char datasize[3];
    unsigned char timestamp[3];
    unsigned char timestamp_ex;
    unsigned char streamid[3];
} FlvTag;

typedef struct 
{
    FILE* fp_;
    int read_pos_;
    int filesize_;
    int reading_tag_data_size_;
} FlvReader;

void FlvReaderOpen(FlvReader* flvReader, const char* filename);

void FlvReaderClose(FlvReader* flvReader);

void FlvReaderRewind(FlvReader* flvReader);

int FlvReaderReadNextTagHeader(FlvReader* flvReader, int* tagType, int* timestamp);

void FlvReaderReadNextTagData(FlvReader* flvReader, char* outBuf);

#endif // _FLV_READER_H_
