#include "file_rtmp_server.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#else
#include <WS2tcpip.h>
#endif
#ifdef __cplusplus  
extern "C" { 
#endif
#include "rtmp_server_base/librtmp/log.h"
#include "rtmp_server_base/librtmp/rtmp_sys.h"

#include "rtmp_server_base/amf_byte_stream.h"
#include "rtmp_server_base/thread.h"

#include "rtmp_server_base/util_tools.h"
#include "rtmp_server_base/simple_mutex.h"

#ifdef __cplusplus  
}  
#endif 
#include "h264_frame_reader.h"
#include "rtmp_connection.h"
#include "lib_rtmp.h"
static const int kSampleRateIndexs[16] = {
	96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025, 8000, 7350
};





FileRtmpServer::FileRtmpServer()
{
	
	int sam_rate_index = 0;
	int i = 0;

	

	server_socket_ = 0;
	state = STATE_STOPPED;

	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		connections_[i] = NULL;
	}

	SimpleMutexInit(&conn_mtx_);
	stream_id_ = 0;
	frame_reader=new H264FrameReader(this);
	aac_reader=new AACFrameReader(this);
	//flv_recoder;
	//frame_reader->H264FrameReader_Init( "test.264", 1);
	frame_reader->H264FrameReader_Init( "W:\\Longse\\testMediaFiles\\S200.h264", 1);
	//aac_reader->AACFrameReader_Init( "test.aac", 1);
	//aac_reader->AACFrameReader_Parse( &sam_rate_index, &channel);
	//sample_rate = kSampleRateIndexs[sam_rate_index];

	//sample_index = sam_rate_index;

	
}

FileRtmpServer::~FileRtmpServer()
{
	int i = 0;



	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] != NULL)
		{
			delete connections_[i];
			connections_[i] = NULL;
		}
	}

	SimpleMutexFree(&conn_mtx_);

	delete frame_reader;
	delete  aac_reader;

}


int FileRtmpServer::RtmpServer_Start(const char *address, 
	unsigned short port)
{
	struct sockaddr_in addr;
	int sockfd, tmp;

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, couldn't create socket", __FUNCTION__);
		return -1;
	}

	tmp = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
		(char *) &tmp, sizeof(tmp) );

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(address);	//htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) ==
		-1)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, TCP bind failed for port number: %d", __FUNCTION__,
			port);
		return -2;
	}

	if (listen(sockfd, 10) == -1)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, listen failed", __FUNCTION__);
		closesocket(sockfd);
		return -3;
	}

	server_socket_ = sockfd;
	state = STATE_WORKING;

	
	frame_reader->H264FrameReader_Start();

	//aac_reader->AACFrameReader_Start();

	//FlvRecoder_Init(&flv_recoder, "test.flv", rtmpServer);
	//FlvRecoder_Start(&flv_recoder);

	ThreadCreate(ServerMainProc, this);

	ThreadCreate(AcceptProc, this);

	return 0;
}

void FileRtmpServer::RtmpServer_Stop()
{


	if (state != STATE_STOPPED)
	{
		state = STATE_STOPPING;

		// wait for streaming threads to exit
		while (state != STATE_STOPPED)
			msleep(10);

		//shutdown(server_socket_, SD_BOTH);
		if (closesocket(server_socket_))
			RTMP_Log(RTMP_LOGERROR, "%s: Failed to close listening socket, error %d",
			__FUNCTION__, GetSockError());

		state = STATE_STOPPED;
	}
}

void FileRtmpServer::CheckConnections()
{
	int i; int conn_count = 0;
	rtmp_bool_t need_print = rtmp_false;

	SimpleMutexLock(&conn_mtx_);
	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] && 
			connections_[i]->RtmpConnection_IsOpen() == rtmp_false)
		{
			delete connections_[i];
			printf("connection closed\n");

			connections_[i] = NULL;
			need_print = rtmp_true;
		}
	}
	if (need_print)
	{
		for (i = 0; i < MAX_CONNECTION; ++i)
		{
			if (connections_[i])
			{
				conn_count++;
			}
		}
	}
	SimpleMutexUnlock(&conn_mtx_);
	if (need_print)
		printf("connection count: %d\n", conn_count);
}

TFTYPE FileRtmpServer::RtmpServerMainThread()
{
	

	unsigned long max_size = 1280 * 720 * 3;
	char* video_buf = (char*)malloc(max_size*10);
	char* audio_buf = (char*)malloc(1024*6*4);
	//if (rtmpServer->frame_reader.sps_size
	for(;;)
	{
		if (frame_reader->sps_size && frame_reader->pps_size)
		{
			break;
		}
		else
		{
			MySleep(100);
		}
	}

	for(;;)
	{
		int has_data = 0;
		if (state == STATE_STOPPING)
			break;

		CheckConnections();

		MySleep(300);
	}

	state = STATE_STOPPED;
}

int FileRtmpServer::GetCount()
{
	int conn_count = 0, i = 0;
	SimpleMutexLock(&conn_mtx_);
	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] != NULL)
		{
			conn_count++;
		}
	}
	SimpleMutexUnlock(&conn_mtx_);
	return conn_count;
}



TFTYPE FileRtmpServer::AcceptThread()
{


	while (state == STATE_WORKING)
	{
		fd_set fds;
		struct timeval tv;
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(struct sockaddr_in);

		FD_ZERO(&fds);
		FD_SET(server_socket_, &fds);
		tv.tv_sec = 10;
		tv.tv_usec = 0;
		//printf("selecting\n");
		if (select(server_socket_ + 1, &fds, NULL, NULL, &tv) <= 0)
		{
			//printf("select failed\n");
		}
		else if (FD_ISSET(server_socket_, &fds))
		{
			//printf("accepting..\n");
			int client_sockfd = accept(server_socket_, (struct sockaddr *) &addr, &addrlen);

			if (client_sockfd > 0)
			{
#if 0
				ConnParam* param = (ConnParam*)malloc(sizeof(ConnParam));

				printf("\n%s: accepted connection from %s\n", __FUNCTION__,
					inet_ntoa(addr.sin_addr));

				param->cliendfd = client_sockfd;
				param->pclass = server;
				ThreadCreate(ConnectionThread, param);
				printf("%s: processed request\n", __FUNCTION__);
#else
				int count = GetCount();

				printf("\n%s: accepted connection from %s\n", __FUNCTION__,
					inet_ntoa(addr.sin_addr));

				if (count < MAX_CONNECTION)
				{
					RtmpConnection* conn = new RtmpConnection(this,client_sockfd);;
					char name[20];
					sprintf(name, "rtmp_%d.log", stream_id_++);
				
					conn->RtmpConnection_Open(name);
					printf("%s: processed request\n", __FUNCTION__);
				}
				else
				{
					printf("!!! Connection Stack is full [%d]\n", count);
				}
#endif
			}
			else
			{
				printf("%s: accept failed", __FUNCTION__);
			}
		}
	}
	state = STATE_STOPPED;
	TFRET();
}

TFTYPE FileRtmpServer::ConnectionThread(void *arg)
{
	fd_set fds;
	struct timeval tv;

	ConnParam* param = (ConnParam*)arg;
	FileRtmpServer* server = param->pclass;
	int clientSocket = param->cliendfd;
	free(param);

	//state = STREAMING_IN_PROGRESS;

	memset(&tv, 0, sizeof(struct timeval));
	tv.tv_sec = 5;

	FD_ZERO(&fds);
	FD_SET(clientSocket, &fds);

	if (select(clientSocket + 1, &fds, NULL, NULL, &tv) <= 0)
	{
		printf("Request timeout/select failed, ignoring request");
	}
	else
	{
		RtmpConnection* conn = new RtmpConnection(this,clientSocket);
		char name[20];
		sprintf(name, "rtmp_%d.log", stream_id_++);
		
		conn->RtmpConnection_Open( name);
		conn->RtmpConnection_Diapatch();

		if (state == STATE_STOPPED || state == STATE_STOPPING)
		{
			
			delete conn;
			return;
		}

		for(;;)
		{
			if (state == STATE_STOPPED ||
				state == STATE_STOPPING)
				return;

			if (conn->RtmpConnection_IsStartPlaying())
			{
				break;
			}
			MySleep(100);
		}

		RtmpServer_OnNewConnection(conn);
	}
	TFRET();
}

void FileRtmpServer::RtmpServer_OnNewConnection( RtmpConnection* conn)
{
	int i; int conn_count = 0;

	conn->RtmpConnection_SendStart();
	conn->RtmpConnection_SendAVCHeader( 
		frame_reader->sps_buf,
		frame_reader->sps_size,
		frame_reader->pps_buf,
		frame_reader->pps_size);
	conn->RtmpConnection_SendAACHeader( 
		sample_rate,
		channel);


	SimpleMutexLock(&conn_mtx_);
	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] == NULL)
		{
			connections_[i] = conn;
			break;
		}
	}
	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] != NULL)
		{
			conn_count++;
		}
	}
	SimpleMutexUnlock(&conn_mtx_);

	printf("connection count: %d\n", conn_count);
}

void FileRtmpServer::RtmpServer_OnAACData( 
	char* buf, int bufLen)
{
	int i = 0;
	RefCountBuf* refbuf = RefCountBuf_New(bufLen+2);
	char* pbuf = refbuf->data_;

	unsigned char flag = 0;
	flag = (10 << 4) |  // soundformat "10 == AAC"
		(3 << 2) |      // soundrate   "3  == 44-kHZ"
		(1 << 1) |      // soundsize   "1  == 16bit"
		1;              // soundtype   "1  == Stereo"

	pbuf = UI08ToBytes(pbuf, flag);
	pbuf = UI08ToBytes(pbuf, 1);    // aac packet type (1, raw)

	memcpy(pbuf, buf, bufLen);
	pbuf += bufLen;

	SimpleMutexLock(&conn_mtx_);
	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] && 
			connections_[i]->RtmpConnection_IsSendHeader())
		{
			connections_[i]->RtmpConnection_PushData(
				refbuf, 0, 0);
		}
	}
	SimpleMutexUnlock(&conn_mtx_);

	RefCountBuf_Delete(refbuf);
}

void FileRtmpServer::RtmpServer_OnH264Data(
	char* buf, int bufLen, int isKeyframe)
{
	int i = 0;
	RefCountBuf* refbuf = RefCountBuf_New(bufLen+5);
	char* pbuf = refbuf->data_;

	if (isKeyframe) pbuf = UI08ToBytes(pbuf, 0x17);
	else pbuf = UI08ToBytes(pbuf, 0x27);
	pbuf = UI08ToBytes(pbuf, 1);    // avc packet type (0, nalu)
	pbuf = UI24ToBytes(pbuf, 0);    // composition time

	memcpy(pbuf, buf, bufLen);
	pbuf += bufLen;

	SimpleMutexLock(&conn_mtx_);
	for (i = 0; i < MAX_CONNECTION; ++i)
	{
		if (connections_[i] && 
			connections_[i]->RtmpConnection_IsSendHeader())
		{
			connections_[i]->RtmpConnection_PushData(
				refbuf, 1, isKeyframe);
		}
	}
	SimpleMutexUnlock(&conn_mtx_);

	RefCountBuf_Delete(refbuf);
}
