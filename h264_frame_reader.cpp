#include "h264_frame_reader.h"

#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus  
extern "C" { 
#endif
#include "rtmp_server_base/librtmp/rtmp_sys.h"

#include "h264_frame_parser.h"
#include "rtmp_server_base/thread.h"
#include "rtmp_server_base/util_tools.h"
#ifdef __cplusplus  
}  
#endif 
#include "file_rtmp_server.h"


H264FrameReader::H264FrameReader(FileRtmpServer * own):m_pown(own)
{

}
H264FrameReader::~H264FrameReader()
{

}

void H264FrameReader::H264FrameReader_Init(const char* filename, int isRepeat)
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


	sps_size = 0;
	pps_size = 0;

	is_stop_ = 0;
}

void H264FrameReader::H264FrameReader_Free()
{
	free(filebuf_);
}

int H264FrameReader::H264FrameReader_ReadFrame(char* outBuf, int* outBufSize)
{
	char* pbufout = 0;
	const char *p = 0;
	const char *end = 0;
	const char *nal_start, *nal_end;

	int has_video_nal = 0;

	char startcodebuf[] = {0x00, 0x00, 0x00, 0x01};
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

	pbufout = outBuf;
	p = pbuf_;
	end = filebuf_ + filesize_;

	nal_start = AVCFindStartCode(p, end);
	while (nal_start < end)
	{
		unsigned int nal_size = 0;
		unsigned char nal_type = 0;

		while(!*(nal_start++));

		nal_end = AVCFindStartCode(nal_start, end);

		nal_size = nal_end - nal_start;
		nal_type = nal_start[0] & 0x1f;

		if (nal_type == 7)        /* SPS */
		{
			has_video_nal = 0;
		}
		else if (nal_type == 8)   /* PPS */
		{
			has_video_nal = 0;
		}
		else
		{
			has_video_nal = 1;
		}

		memcpy(pbufout, startcodebuf, 4);
		pbufout += 4;
		memcpy(pbufout, nal_start, nal_size);
		pbufout += nal_size;

		nal_start = nal_end;

		if (has_video_nal)
		{
			break;
		}
	}

	*outBufSize = pbufout - outBuf;
	pbuf_ = nal_start;

	return 1;
}

void H264FrameReader::RunH264FrameReaderPthread()
{


	unsigned long max_size = 1280 * 720;
	unsigned char* yuvbuf = (unsigned char*)malloc(max_size);
	char* x264buf = (char*)malloc(max_size*10);
	char* tmpbuf = (char*)malloc(max_size*10);
	int x264buf_len = 0;
	int tmpbuf_len = 0;

	while (!is_stop_)
	{
		unsigned int now_tick = GetCurrentTickCount();
		unsigned int next_tick = now_tick + 41;     // 10fps

		if (H264FrameReader_ReadFrame( tmpbuf, &tmpbuf_len))
		{
			int is_keyframe = 0;

			// 接收到h264裸流，开始处理
			if (sps_size == 0)
			{
				ParseH264Frame(tmpbuf, tmpbuf_len, x264buf, &x264buf_len, 
					sps_buf, &sps_size,
					pps_buf, &pps_size, 
					&is_keyframe);
			}
			else
			{
				ParseH264Frame(tmpbuf, tmpbuf_len, x264buf, &x264buf_len, 
					NULL, &sps_size,
					NULL, &pps_size, 
					&is_keyframe);
			}

			
			m_pown->RtmpServer_OnH264Data(x264buf, x264buf_len, is_keyframe);
		}
		else
		{
			break;
		}

		now_tick = GetCurrentTickCount();
		if (next_tick > now_tick)
		{
			MySleep(next_tick-now_tick);
		}
	}

	free(yuvbuf);
	free(x264buf);
	free(tmpbuf);

	pbuf_ = 0;

	TFRET();
}

void H264FrameReader::H264FrameReader_Start()
{
	is_stop_ = 0;
	ThreadCreate(H264FrameReaderProc, this);
}

void H264FrameReader::H264FrameReader_Stop()
{
	is_stop_ = 1;
	while (1)
	{
		if (pbuf_ == NULL)
			break;
		MySleep(100);
	}
}


