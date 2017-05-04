#ifndef _H264_FRAME_PARSER_H_
#define _H264_FRAME_PARSER_H_

const char* AVCFindStartCodeInternal(const char *p, const char *end);

const char* AVCFindStartCode(const char *p, const char *end);

void AVCParseNalUnits(const char *bufIn, int inSize, char* bufOut, int* outSize);

void ParseH264Frame(const char* nalsbuf, int size, char* outBuf, int* outLen,
    char* spsBuf, int* spsSize, char* ppsBuf, int* ppsSize,
    int* isKeyframe);

#endif // _H264_FRAME_PARSER_H_
