#include "h264_frame_parser.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "rtmp_server_base/amf_byte_stream.h"

const char* AVCFindStartCodeInternal(const char *p, const char *end)
{
    const char *a = p + 4 - ((ptrdiff_t)p & 3);

    for (end -= 3; p < a && p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    for (end -= 3; p < end; p += 4) {
        unsigned int x = *(const unsigned int*)p;
        //      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
        //      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
        if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
            if (p[1] == 0) {
                if (p[0] == 0 && p[2] == 1)
                    return p;
                if (p[2] == 0 && p[3] == 1)
                    return p+1;
            }
            if (p[3] == 0) {
                if (p[2] == 0 && p[4] == 1)
                    return p+2;
                if (p[4] == 0 && p[5] == 1)
                    return p+3;
            }
        }
    }

    for (end += 3; p < end; p++) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1)
            return p;
    }

    return end + 3;
}

const char* AVCFindStartCode(const char *p, const char *end)
{
    const char *out= AVCFindStartCodeInternal(p, end);
    if(p<out && out<end && !out[-1]) out--;
    return out;
}

void AVCParseNalUnits(const char *bufIn, int inSize, char* bufOut, int* outSize)
{
    const char *p = bufIn;
    const char *end = p + inSize;
    const char *nal_start, *nal_end;

    char* pbuf = bufOut;

    *outSize = 0;
    nal_start = AVCFindStartCode(p, end);
    while (nal_start < end)
    {
        unsigned int nal_size = 0;

        while(!*(nal_start++));

        nal_end = AVCFindStartCode(nal_start, end);

        nal_size = nal_end - nal_start;
        pbuf = UI32ToBytes(pbuf, nal_size);
        memcpy(pbuf, nal_start, nal_size);
        pbuf += nal_size;

        nal_start = nal_end;
    }

    *outSize = (pbuf - bufOut);
}

void ParseH264Frame(const char* nalsbuf, int size, char* outBuf, int* outLen,
    char* spsBuf, int* spsSize, char* ppsBuf, int* ppsSize,
    int* isKeyframe)
{
    char* start = 0;
    char* end = 0;

    AVCParseNalUnits(nalsbuf, size, (char*)outBuf, outLen);

    start = outBuf;
    end = outBuf + *outLen;

    /* look for sps and pps */
    while (start < end) 
    {
        unsigned int size = BytesToUI32(start);
        unsigned char nal_type = start[4] & 0x1f;

        if (nal_type == 7 && spsBuf)        /* SPS */
        {
            *spsSize = size;
            memcpy(spsBuf, start + 4, *spsSize);
        }
        else if (nal_type == 8 && ppsBuf)   /* PPS */
        {
            *ppsSize = size;
            memcpy(ppsBuf, start + 4, *ppsSize);
        }
        else if (nal_type == 5)
        {
            *isKeyframe = 1;
        }
        start += size + 4;
    }
}
