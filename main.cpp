
#include "file_rtmp_server.h"
#include "rtmp_server_base/librtmp/rtmp_sys.h"

#ifdef WIN32
#include <Winsock2.h>

#define InitSockets()	{\
	WORD version;			\
	WSADATA wsaData;		\
	\
	version = MAKEWORD(1,1);	\
	WSAStartup(version, &wsaData);	}

#define	CleanupSockets()	WSACleanup()

#else
#include<signal.h>

#define InitSockets()
#define	CleanupSockets()

void handle_pipe(int sigid)
{
}

#endif

int main(int argc, char** argv)
{
	FileRtmpServer* file_rtmpserver = new FileRtmpServer();

#ifndef WIN32
	struct sigaction action;
	action.sa_handler = handle_pipe;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGPIPE, &action, NULL);
#endif

	InitSockets();

	printf("---------------------\nrtmp_file_server 1.1\n---------------------\n");


	if (argc == 2)
		file_rtmpserver->RtmpServer_Start( argv[1], 1935);
	else
		file_rtmpserver->RtmpServer_Start( "0.0.0.0", 1935);

	for (;;)
	{
		char cmd[50];
		scanf("%s", cmd);
		if (strcmp(cmd, "exit") == 0)
		{
			break;
		}
	}

	file_rtmpserver->RtmpServer_Stop();
	delete file_rtmpserver;

	CleanupSockets();

	return 0;
}
