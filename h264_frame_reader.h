#ifndef _H264_FRAME_READER_H_
#define _H264_FRAME_READER_H_

#include <stdio.h>
#include <assert.h>


class FileRtmpServer;
class H264FrameReader
{
	friend class FileRtmpServer;
public:		

	H264FrameReader(FileRtmpServer * own);
	~H264FrameReader();
	void H264FrameReader_Init(const char* filename, int isRepeat);

	void H264FrameReader_Free();

	void H264FrameReader_Start();

	void H264FrameReader_Stop();


	int H264FrameReader_ReadFrame(char* outBuf, int* outBufSize);
  private:
		   
		    static void H264FrameReaderProc(void *arg)
		    {
			    H264FrameReader *reader = static_cast<H264FrameReader *>(arg);
			    assert(reader != NULL);			   
			    reader->RunH264FrameReaderPthread();
		    }
	 
		    void   RunH264FrameReaderPthread();
private:
	char sps_buf[1024];
	int sps_size;
	char pps_buf[1024];
	int pps_size;

	char* filebuf_;
	const char* pbuf_;
	int filesize_;
	int is_repeat_;
	unsigned char is_stop_;

	FileRtmpServer *m_pown;


};




#endif // _H264_FRAME_READER_H_
