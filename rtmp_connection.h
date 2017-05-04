/*******************************************************************************
 * rtmp_connection.h
 * Copyright: (c) 2013 Haibin Du(haibinnet@qq.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 *
 ******************************************************************************/

#ifndef _HBASE_RTMP_LIVE_CONNECTION_H_
#define _HBASE_RTMP_LIVE_CONNECTION_H_
#ifdef __cplusplus  
extern "C" { 
#endif
#include "rtmp_server_base/simple_mutex.h"

#include "data_list.h"
#ifdef __cplusplus  
}  
#endif 
#include <assert.h>
class LibRtmpServer;
class FileRtmpServer;

typedef enum rtmp_bool
{
    rtmp_true  = 1,
    rtmp_false = 0
} rtmp_bool_t;

class RtmpConnection
{
			friend class FileRtmpServer;
public:
		RtmpConnection(FileRtmpServer *pown,int clientSocket);
		~RtmpConnection();
		void RtmpConnection_Open(const char* logfilename);

		void RtmpConnection_Close();

		void RtmpConnection_PushData(RefCountBuf* refbuf,
			int isVideo, int isKeyframe);

		rtmp_bool_t RtmpConnection_IsOpen();

		void RtmpConnection_Diapatch();

		rtmp_bool_t RtmpConnection_OnRead();

		void RtmpConnection_OnClientPlay(const char* playPath);

		void RtmpConnection_OnClientPause();

		rtmp_bool_t RtmpConnection_IsStartPlaying();

		rtmp_bool_t RtmpConnection_IsSendHeader();

		rtmp_bool_t RtmpConnection_IsPause();

		void RtmpConnection_SendStart();

		void RtmpConnection_SendStop();

		void RtmpConnection_SendAVCHeader(
			const char* spsBuf, int spsSize,
			const char* ppsBuf, int ppsSize);

		void RtmpConnection_SendAACHeader(
			int sampleRate, int channel);

		int RtmpConnection_SendData(
			const char* buf, int bufLen, int type);

		int RtmpConnection_SocketID();

	private:
			
		private:
		    int H264FrameReader_ReadFrame(char* outBuf, int* outBufSize);
		    static void RtmpConnectionProc(void *arg)
		    {
			    RtmpConnection *conn = static_cast<RtmpConnection *>(arg);
			    assert(conn != NULL);			   
			    conn->RunRtmpConnectionPthread();
		    }
	 
		    void   RunRtmpConnectionPthread();

private:
			rtmp_bool_t is_open_;
			rtmp_bool_t is_closing_;
			rtmp_bool_t is_has_send_header_;
			rtmp_bool_t is_start_playing_;
			rtmp_bool_t is_pause_;

			LibRtmpServer* librtmp_server_;
			int socket_;

			DataList* data_list_;
			SimpleMutex data_mtx_;
			volatile int has_data_;

			FileRtmpServer *m_pown;

};






#endif // _HBASE_RTMP_LIVE_CONNECTION_H_
