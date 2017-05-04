#ifndef _FILE_RTMP_SERVER_H_
#define _FILE_RTMP_SERVER_H_
#ifdef __cplusplus  
extern "C" { 
#endif
#include "rtmp_server_base/librtmp/rtmp.h"
#include "rtmp_server_base/simple_mutex.h"

#include "h264_frame_reader.h"
#include "aac_frame_reader.h"
#include "data_list.h"
#ifdef __cplusplus  
}  
#endif 
enum RtmpServerState
{
    STATE_WORKING,
    STATE_STOPPING,
    STATE_STOPPED
};

#define MAX_CONNECTION 15
typedef struct ConnParam 
{
	FileRtmpServer* pclass;
	int cliendfd;
} ConnParam;
class LibRtmpServer;
class RtmpConnection;
class H264FrameReader;
class AACFrameReader;

class FileRtmpServer
{
public:
			FileRtmpServer();
			~FileRtmpServer();
			/***
			 * ����RtmpServer
			 * @param rtmpServer: rtmp serverģ��ָ��
			 * @param address: rtmp server������ip��ַ�ַ���
			 * @param port: rtmp server�󶨵�tcp�˿�
			 * @returns: �����Ƿ�ɹ����ɹ�����0�����򷵻ظ�ֵ
			 */
			int RtmpServer_Start( const char *address, 
				unsigned short port);
			

			/***
			 * �ر�RtmpServer����ȴ����̹߳ر�
			 */
			void RtmpServer_Stop();
			void CheckConnections();
			/***
			 * �ص�����
			 * �����µĿͻ�������ʱ
			 */
			void RtmpServer_OnNewConnection(RtmpConnection* conn);

			void RtmpServer_OnAACData(char* buf, int bufLen);

			void RtmpServer_OnH264Data(char* buf, int bufLen, int isKeyframe);

			int GetCount();

			static void ServerMainProc(void *arg)
			{
				FileRtmpServer *recorder = static_cast<FileRtmpServer *>(arg);
				assert(recorder != NULL);
				
				recorder->RtmpServerMainThread();
			}
			
			static void ConnectionProc(void *arg)
			{
				ConnParam *recorder = static_cast<ConnParam *>(arg);
				assert(recorder != NULL);

				recorder->pclass->ConnectionThread(recorder);
			}

			static void AcceptProc(void *arg)
			{
				FileRtmpServer *recorder = static_cast<FileRtmpServer *>(arg);
				assert(recorder != NULL);

				recorder->AcceptThread();
			}
		

			TFTYPE RtmpServerMainThread();
			TFTYPE ConnectionThread(void *arg);
			TFTYPE AcceptThread();
public:

			int server_socket_;
			int state;

			RtmpConnection* connections_[MAX_CONNECTION];
			SimpleMutex conn_mtx_;
			int stream_id_;

			//FlvRecoder flv_recoder;
			H264FrameReader* frame_reader;
			AACFrameReader* aac_reader;
			int sample_rate;
			int channel;
			int sample_index;
} ;





#endif // _FILE_RTMP_SERVER_H_
