#ifndef _AAC_FRAME_READER_H_
#define _AAC_FRAME_READER_H_

#include <stdio.h>
#include <assert.h>


class FileRtmpServer;

class AACFrameReader{
public:
	AACFrameReader(FileRtmpServer * owner);
	~AACFrameReader();




	void AACFrameReader_Init(const char* filename, int isRepeat);

	void AACFrameReader_Free();

	int AACFrameReader_Parse(int* samRate, int* channel);

	int AACFrameReader_ReadFrame(char* outBuf, int* outBufSize);

	void AACFrameReader_Start();

	void AACFrameReader_Stop();
public:
	int H264FrameReader_ReadFrame(char* outBuf, int* outBufSize);
	static void AACFrameReaderProc(void *arg)
	{
		AACFrameReader *reader = static_cast<AACFrameReader *>(arg);
		assert(reader != NULL);			   
		reader->RunAACFrameReaderPthread();
	}

	void   RunAACFrameReaderPthread();

public:
	char* filebuf_;
	const char* pbuf_;
	int filesize_;
	int is_repeat_;
	unsigned char is_stop_;

	FileRtmpServer *m_pown;

};




#endif // _AAC_FRAME_READER_H_
