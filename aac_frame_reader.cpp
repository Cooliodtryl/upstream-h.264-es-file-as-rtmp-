#include "aac_frame_reader.h"

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus  
extern "C" { 
#endif
#include "rtmp_server_base/bit_reader.h"
#include "rtmp_server_base/thread.h"
#include "rtmp_server_base/util_tools.h"
#ifdef __cplusplus  
}  
#endif 
#include "file_rtmp_server.h"
AACFrameReader::AACFrameReader( FileRtmpServer *pown):m_pown(pown)
{
		
}
AACFrameReader::~AACFrameReader()
{
	    AACFrameReader_Free();
} 
void AACFrameReader::AACFrameReader_Init( const char* filename, int isRepeat)
{
	FILE* fp = fopen(filename, "rb");
	is_repeat_ = isRepeat;
	filebuf_ = 0;
	filesize_ = 0;

	if (fp)
	{
		int retval = 0;
		fseek(fp, 0, SEEK_END);
		filesize_ = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		filebuf_ = (char*)malloc(filesize_);
		retval = fread(filebuf_, 1, filesize_, fp);

		fclose(fp);
	}
	pbuf_ = filebuf_;

	is_stop_ = 0;
	
}

void AACFrameReader::AACFrameReader_Free()
{
	free(filebuf_);
}

int AACFrameReader::AACFrameReader_Parse( int* samRate, int* channel)
{
	const char *p = 0;
	const char *end = 0;

	BitReader bit_reader;

	p = pbuf_;
	end = filebuf_ + filesize_;

	BitReader_Init(&bit_reader, p);
	while (BitReader_ShowBitUI32(&bit_reader, 12) != 0xfff)
	{
		BitReader_SkipBytes(&bit_reader, 1);
	}

	pbuf_ = BitReader_SrcBuf(&bit_reader) + BitReader_BytePos(&bit_reader);

	BitReader_ReadBitUI32(&bit_reader, 12);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 2);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 2);
	*samRate = BitReader_ReadBitUI32(&bit_reader, 4);
	BitReader_ReadBitUI32(&bit_reader, 1);
	*channel = BitReader_ReadBitUI32(&bit_reader, 3);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 1);

	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 13);
	BitReader_ReadBitUI32(&bit_reader, 11);
	BitReader_ReadBitUI32(&bit_reader, 2);

	return 1;
}

int AACFrameReader::AACFrameReader_ReadFrame( char* outBuf, 
	int* outBufSize)
{
	const char *p = 0;
	const char *end = 0;

	BitReader bit_reader;
	int framelen = 0;

	if (pbuf_ >= filebuf_ + filesize_)
	{
		if (is_repeat_)
		{
			pbuf_ = filebuf_;
		}
		else
		{
			return 0;
		}
	}

	p = pbuf_;
	end = filebuf_ + filesize_;

	BitReader_Init(&bit_reader, p);
	while (BitReader_ShowBitUI32(&bit_reader, 12) != 0xfff)
	{
		BitReader_SkipBytes(&bit_reader, 1);
	}

	BitReader_ReadBitUI32(&bit_reader, 12);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 2);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 2);
	BitReader_ReadBitUI32(&bit_reader, 4);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 3);
	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 1);

	BitReader_ReadBitUI32(&bit_reader, 1);
	BitReader_ReadBitUI32(&bit_reader, 1);
	framelen = BitReader_ReadBitUI32(&bit_reader, 13);
	BitReader_ReadBitUI32(&bit_reader, 11);
	BitReader_ReadBitUI32(&bit_reader, 2);

	p = BitReader_SrcBuf(&bit_reader) + BitReader_BytePos(&bit_reader);
	memcpy(outBuf, p, framelen-7);
	p += (framelen-7);

	*outBufSize = framelen-7;
	pbuf_ = p;

	return 1;
}

static const int kSampleRateIndexs[16] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000, 7350
};

	void AACFrameReader::RunAACFrameReaderPthread()
	{
	   

	int frame_interval = 0;

	unsigned long max_size = 1024*6*4;
	char* tmpbuf = (char*)malloc(max_size);
	int tmpbuf_len = 0;

	int playing_time = GetCurrentTickCount();
	int begin_timestamp = playing_time;

	{   // 获取采样率
		int channel;
		int sam_rate_index;
		int sample_rate;
		AACFrameReader_Parse(&sam_rate_index, &channel);
		sample_rate = kSampleRateIndexs[sam_rate_index];
		frame_interval = 1000.0 / (sample_rate / 1024.0);
	}

	    while (!is_stop_)
	    {
		if (AACFrameReader_ReadFrame(tmpbuf, &tmpbuf_len))
		{
		    // 接收到aac裸流, 保存当前帧
		    m_pown->RtmpServer_OnAACData(tmpbuf, tmpbuf_len);
		}
		else
		{
			break;
		}

		// 计算等待下一帧的时间
		{
			int real_time = GetCurrentTickCount();
			int diff = real_time - playing_time;
			if (frame_interval > diff)
				MySleep(frame_interval - diff);
		}

		playing_time += frame_interval;
	    }

	    free(tmpbuf);

	    pbuf_ = 0;

	    return ;
	}

void AACFrameReader::AACFrameReader_Start()
{
	is_stop_ = 0;
	ThreadCreate(AACFrameReaderProc, this);
}

void AACFrameReader::AACFrameReader_Stop()
{
	is_stop_ = 1;
	for (;;)
	{
		if (pbuf_ == NULL)
			break;

		MySleep(100);
	}
}
