/*******************************************************************************
 * rtmp_connection.c
 * Copyright: (c) 2013 Haibin Du(haibinnet@qq.com). All rights reserved.
 * -----------------------------------------------------------------------------
 *
 ******************************************************************************/

#include "rtmp_connection.h"

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
#include "rtmp_server_base/amf_byte_stream.h"

#include "rtmp_server_base/librtmp/log.h"
#include "rtmp_server_base/thread.h"
#include "rtmp_server_base/util_tools.h"
#ifdef __cplusplus  
}  
#endif 


#include "lib_rtmp.h"
#include "file_rtmp_server.h"




RtmpConnection::RtmpConnection(FileRtmpServer *pown,int clientSocket):m_pown(pown)
{

    is_open_ = rtmp_false;
    is_closing_ = rtmp_false;
    is_has_send_header_ = rtmp_false;
    is_start_playing_ = rtmp_false;
    is_pause_ = rtmp_false;
    librtmp_server_ = NULL;
    socket_ = clientSocket;

    data_list_ = DataList_Create();
    SimpleMutexInit(&data_mtx_);
    has_data_ = 0;

}

RtmpConnection::~RtmpConnection()
{
    RtmpConnection_Close();

    DataList_Destroy(data_list_);
    data_list_ = NULL;
    SimpleMutexFree(&data_mtx_);

   
}

void RtmpConnection::RtmpConnection_Open(const char* logfilename)
{
	if (is_open_) return;

	librtmp_server_ = new LibRtmpServer(this);

	
	if (!librtmp_server_->LibRtmpOpen(socket_, logfilename))
	{
		RTMP_Log(RTMP_LOGERROR, "Open Handshake failed");
		return;
	}

	ThreadCreate(RtmpConnectionProc, this);

 
}

void RtmpConnection::RtmpConnection_Close()
{
    if (is_closing_) return;

    is_closing_ = rtmp_true;

    while (is_open_ == rtmp_true)
    {
        MySleep(100);
    }

    librtmp_server_->LibRtmpClose();
    delete librtmp_server_;
    librtmp_server_ = NULL;
}

void RtmpConnection::RtmpConnection_PushData(RefCountBuf* refbuf,
    int isVideo, int isKeyframe)
{
    SimpleMutexLock(&data_mtx_);
    DataList_Pushback(data_list_, refbuf, isVideo, isKeyframe);
    SimpleMutexUnlock(&data_mtx_);

    has_data_ = 1;
}

TFTYPE RtmpConnection::RunRtmpConnectionPthread()
{
  

    is_open_ = rtmp_true;

    for (;;)
    {
        int has_data = 0;

        fd_set read_fds;
        fd_set write_fds;
        struct timeval tv;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(socket_, &read_fds);
        FD_SET(socket_, &write_fds);
        memset(&tv, 0, sizeof(struct timeval));

        
        if (is_closing_ == rtmp_true)
        {
            break;
        }

        if (select(socket_ + 1, &read_fds, &write_fds, NULL, &tv) <= 0)
        {
            MySleep(10);
            continue;
        }
	//printf("[%s:%d]拜托,能不能先走到这里来呢????,is_has_send_header_:%d\n",__FUNCTION__,__LINE__,is_has_send_header_);  
        if (!is_has_send_header_)
        {
            RtmpConnection_Diapatch();
            for(;;)
            {
                if (is_closing_ == rtmp_true)
                    goto CONNECTION_END;
                if (is_start_playing_)
                {
					printf("[%s:%d]startintg to play suuceffulaty\n",__FUNCTION__,__LINE__);
					break;
				}
                MySleep(100);
            }

            m_pown->RtmpServer_OnNewConnection(this);

        }

        if (FD_ISSET(socket_, &read_fds) && !is_has_send_header_)
        {
            rtmp_bool_t ret = RtmpConnection_OnRead();
            if (ret == rtmp_false)
            {
                printf("connection read failed, need close\n");

                goto CONNECTION_END;
            }
            else
            {
                if (is_start_playing_ && !is_has_send_header_)
                {
                    m_pown->RtmpServer_OnNewConnection(this);
                }
            }
        }

        if (has_data_ == 1)
        {
			//printf("[%s:%d]是不是这里做了什么操作????\n",__FUNCTION__,__LINE__);
            DataList* tmplist = NULL;
            DataListNode* node = NULL;

            has_data_ = 0;

            tmplist = DataList_Create();

            SimpleMutexLock(&data_mtx_);
            DataList_Swap(data_list_, tmplist);
            SimpleMutexUnlock(&data_mtx_);

            for (node = DataList_Head(tmplist); node != DataList_End(tmplist);
                node = node->next_)
            {
                if (node->is_video_)
                {
                    if (!RtmpConnection_SendData(node->buf_->data_, 
                        node->buf_->data_len_, FLV_TAG_TYPE_VIDEO))
                    {
                        printf("connection send failed, need close\n");

                        DataList_Destroy(tmplist);
                        goto CONNECTION_END;
                    }
                }
                else
                {
                    RtmpConnection_SendData(node->buf_->data_, 
                        node->buf_->data_len_, FLV_TAG_TYPE_AUDIO);
                }
            }

            DataList_Destroy(tmplist);
        }
        else
        {
            MySleep(100);
        }
    }

CONNECTION_END:
    is_open_ = rtmp_false;

    TFRET();
}

rtmp_bool_t RtmpConnection::RtmpConnection_IsOpen()
{
    return is_open_;
}

void RtmpConnection::RtmpConnection_Diapatch()
{
    RTMPPacket packet = { 0 };

    while (RTMP_IsConnected(librtmp_server_->rtmp) && 
        RTMP_ReadPacket(librtmp_server_->rtmp, &packet))
    {
        if (!RTMPPacket_IsReady(&packet))
            continue;

        librtmp_server_->DispatchRtmpPacket(&packet);

        RTMPPacket_Free(&packet);

        if (is_start_playing_)
        {
            break;
        }
    }
}

rtmp_bool_t RtmpConnection::RtmpConnection_OnRead()
{
    if (is_closing_) return rtmp_false;


    if (RTMP_IsConnected(librtmp_server_->rtmp))
    {
        RTMPPacket packet = { 0 };
        int err = RTMP_ReadPacket(librtmp_server_->rtmp, &packet);
        if (!err)
        {
            RTMP_Log(RTMP_LOGERROR, "RTMP_ReadPacket failed");
            return rtmp_false;
        }
        if (RTMPPacket_IsReady(&packet))
        {
            librtmp_server_->DispatchRtmpPacket( &packet);
            RTMPPacket_Free(&packet);
            return rtmp_true;
        }
    }

    return rtmp_false;
}

void RtmpConnection::RtmpConnection_OnClientPlay(const char* playPath)
{
	printf("客户端选择的播放URL是:[%s]\n",playPath);
    is_start_playing_ = rtmp_true;
}

void RtmpConnection::RtmpConnection_OnClientPause()
{
    if (is_pause_)
    {
        RtmpConnection_SendStart();
//         if (parent_class_)
//             parent_class_->OnPlayRestart(this);
        is_pause_ = rtmp_false;
    }
    else
    {
        RtmpConnection_SendStop();
        is_pause_ = rtmp_true;
    }
}

rtmp_bool_t RtmpConnection::RtmpConnection_IsStartPlaying()
{
    return is_start_playing_;
}

rtmp_bool_t RtmpConnection::RtmpConnection_IsSendHeader()
{
    return RtmpConnection::is_has_send_header_;
}

rtmp_bool_t RtmpConnection::RtmpConnection_IsPause()
{
    return is_pause_;
}

void RtmpConnection::RtmpConnection_SendStart()
{
    if (is_closing_) return;

    librtmp_server_->LibRtmpSendPlayStart();

    librtmp_server_->LibRtmpResetTimestamp();
}

void RtmpConnection::RtmpConnection_SendStop()
{
    if (is_closing_) return;

    librtmp_server_->LibRtmpSendPlayStop();
}

void RtmpConnection::RtmpConnection_SendAVCHeader(
    const char* spsBuf, int spsSize,
    const char* ppsBuf, int ppsSize)
{
    if (is_closing_) return;

    librtmp_server_->LibRtmpSendAVCHeader( spsBuf, spsSize, ppsBuf, ppsSize);

    is_has_send_header_ = rtmp_true;
}

void RtmpConnection::RtmpConnection_SendAACHeader(
    int sampleRate, int channel)
{
    if (is_closing_) return;

    librtmp_server_->LibRtmpSendAACHeader( sampleRate, channel);
}

int RtmpConnection::RtmpConnection_SendData(
    const char* buf, int bufLen, int type)
{
    if (is_closing_) return rtmp_false;

    return librtmp_server_->LibRtmpSendData( buf, bufLen,  type, 
        librtmp_server_->LibRtmpGetTimestamp());
}

int RtmpConnection::RtmpConnection_SocketID()
{
    return socket_;
}