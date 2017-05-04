#ifndef _LIBRTMP_H_
#define _LIBRTMP_H_

#include <stdio.h>
#ifdef __cplusplus  
extern "C" { 
#endif
#include "librtmp/rtmp.h"
#ifdef __cplusplus  
}  
#endif 
class RtmpConnection;
class  LibRtmpServer
{
		friend class RtmpConnection;
public:
		LibRtmpServer(RtmpConnection *pown);
		~LibRtmpServer();
		int LibRtmpOpen(int clientSocket, const char* logfilename);

		void LibRtmpClose();

		void LibRtmpRun();

		int LibRtmpSendPlayStart();

		int LibRtmpSendPlayStop();

		int LibRtmpSendNotifySampleAccess();

		int LibRtmpSendData(const char* buf, int bufLen, 
			int type, unsigned int timestamp);

		int LibRtmpSendAVCHeader(const char* spsBuf, int spsSize,
			const char* ppsBuf, int ppsSize);

		int LibRtmpSendAACHeader(int sampleRate, int channel);

		void LibRtmpResetTimestamp();

		unsigned int LibRtmpGetTimestamp();

		int DispatchRtmpPacket(RTMPPacket *packet);

		int LibRtmpSendPlayReset();
		int ServeInvoke(RTMPPacket *packet, unsigned int offset);
		int SendResultNumber(double txn, double ID);
		int SendConnectResult(double txn);
private:
	RTMP* rtmp;
	FILE* flog;
	int client_socket;
	long long time_begin_;



	RtmpConnection *m_pown;
};






#endif // _LIBRTMP_H_
