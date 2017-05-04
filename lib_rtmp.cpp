#include "lib_rtmp.h"

#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus  
extern "C" { 
#endif
#include "librtmp/log.h"
#include "librtmp/rtmp_sys.h"

#include "bit_writter.h"
#include "util_tools.h"
#ifdef __cplusplus  
}  
#endif 
#include "rtmp_connection.h"
//////////////////////////////////////////////////////////////////////////

#define STR2AVAL(av,str)	av.av_val = str; av.av_len = strlen(av.av_val)

#define SAVC(x) static const AVal av_##x = AVC(#x)

SAVC(app);
SAVC(connect);
SAVC(flashVer);
SAVC(swfUrl);
SAVC(pageUrl);
SAVC(tcUrl);
SAVC(fpad);
SAVC(capabilities);
SAVC(audioCodecs);
SAVC(videoCodecs);
SAVC(videoFunction);
SAVC(objectEncoding);
SAVC(_result);
SAVC(createStream);
SAVC(getStreamLength);
SAVC(play);
SAVC(fmsVer);
SAVC(mode);
SAVC(level);
SAVC(code);
SAVC(description);
SAVC(secureToken);

SAVC(onStatus);
SAVC(status);
static const AVal av_NetStream_Play_Reset = AVC("NetStream.Play.Reset");
static const AVal av_NetStream_Play_Start = AVC("NetStream.Play.Start");
static const AVal av_Started_playing = AVC("Started playing");
static const AVal av_NetStream_Play_Stop = AVC("NetStream.Play.Stop");
static const AVal av_Stopped_playing = AVC("Stopped playing");
static const AVal av_NetStream_Data_Start = AVC("NetStream.Data.Start");
SAVC(details);
SAVC(clientid);
static const AVal av_NetStream_Authenticate_UsherToken = AVC("NetStream.Authenticate.UsherToken");

static const AVal av_dquote = AVC("\"");
static const AVal av_escdquote = AVC("\\\"");
void AVreplace(AVal *src, const AVal *orig, const AVal *repl);

static const AVal av_RtmpSampleAccess = {"|RtmpSampleAccess", sizeof("|RtmpSampleAccess")-1};

SAVC(pause);

//////////////////////////////////////////////////////////////////////////


LibRtmpServer::LibRtmpServer(RtmpConnection *pown):m_pown(pown)
{

}
LibRtmpServer::~LibRtmpServer()
{

}
int LibRtmpServer::LibRtmpOpen(int clientSocket, const char* logfilename)
{
	rtmp = 0;
	flog = 0;
	client_socket = clientSocket;

	if (logfilename)
	{
		flog = fopen(logfilename, "w");
		RTMP_LogSetLevel(RTMP_LOGDEBUG2);
		RTMP_LogSetOutput(flog);
	}

	rtmp = RTMP_Alloc();
	RTMP_Init(rtmp);
	rtmp->m_sb.sb_socket = clientSocket;
	if (!RTMP_Serve(rtmp))
	{
		RTMP_Log(RTMP_LOGERROR, "Handshake failed");
		RTMP_Close(rtmp);
		rtmp = 0;
		return 0;
	}

	return 1;
}

void LibRtmpServer::LibRtmpClose()
{
	if (rtmp)
	{
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		rtmp = 0;
	}

	if (flog)
	{
		fclose(flog);
		flog = 0;
	}
}

void LibRtmpServer::LibRtmpRun()
{
	RTMPPacket packet = { 0 };

	while (RTMP_IsConnected(rtmp) && RTMP_ReadPacket(rtmp, &packet))
	{
		if (!RTMPPacket_IsReady(&packet))
			continue;

		DispatchRtmpPacket( &packet);

		RTMPPacket_Free(&packet);
	}
}

int LibRtmpServer::LibRtmpSendPlayReset()
{
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	char *enc = 0;

	packet.m_nChannel = 0x04;     // control channel (invoke)
	packet.m_headerType = 0; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = 0x05;
	*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Reset);
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Started_playing);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &rtmp->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &av_clientid);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;
	return RTMP_SendPacket(rtmp, &packet, FALSE);
}

int LibRtmpServer::LibRtmpSendPlayStart()
{
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	char *enc = 0;

	RTMP_SendCtrl(rtmp, 0, 1, 0);
	LibRtmpSendPlayReset();

	packet.m_nChannel = 0x14;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = 0x05;
	*enc++ = AMF_OBJECT;
	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Start);
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Started_playing);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &rtmp->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &av_clientid);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;
	return RTMP_SendPacket(rtmp, &packet, FALSE);
}

int LibRtmpServer::LibRtmpSendPlayStop()
{
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	char *enc = 0;

	RTMP_SendCtrl(rtmp, 1, 1, 0);

	packet.m_nChannel = 0x03;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_onStatus);
	enc = AMF_EncodeNumber(enc, pend, 0);
	*enc++ = 0x05;
	*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Stop);
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Stopped_playing);
	enc = AMF_EncodeNamedString(enc, pend, &av_details, &rtmp->Link.playpath);
	enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &av_clientid);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;
	return RTMP_SendPacket(rtmp, &packet, FALSE);
}

int LibRtmpServer::LibRtmpSendNotifySampleAccess()
{
	RTMPPacket packet;
	char pbuf[512], *pend = pbuf+sizeof(pbuf);
	char *enc = 0;

	RTMP_SendCtrl(rtmp, 0, 1, 0);
	LibRtmpSendPlayReset();

	packet.m_nChannel = 0x14;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INFO;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 1;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av_RtmpSampleAccess);
	*enc++ = 0x01;
	*enc++ = 0x00;
	*enc++ = 0x01;
	*enc++ = 0x00;

	packet.m_nBodySize = enc - packet.m_body;
	return RTMP_SendPacket(rtmp, &packet, FALSE);
}

int LibRtmpServer::LibRtmpSendData(const char* buf, int bufLen, 
	int type, unsigned int timestamp)
{
	int ret = 0;
	RTMPPacket rtmp_pakt;
	RTMPPacket_Reset(&rtmp_pakt);
	RTMPPacket_Alloc(&rtmp_pakt, bufLen);

	rtmp_pakt.m_packetType = (uint8_t)type;
	rtmp_pakt.m_nBodySize = bufLen;
	rtmp_pakt.m_nTimeStamp = timestamp;
	rtmp_pakt.m_nChannel = 0x06;
	rtmp_pakt.m_headerType = RTMP_PACKET_SIZE_LARGE;
	rtmp_pakt.m_nInfoField2 = 1;
	memcpy(rtmp_pakt.m_body, buf, bufLen);

	ret = RTMP_SendPacket(rtmp, &rtmp_pakt, 0);
	RTMPPacket_Free(&rtmp_pakt);

	return ret;
}

int LibRtmpServer::LibRtmpSendAVCHeader(const char* spsBuf, int spsSize,
	const char* ppsBuf, int ppsSize)
{
	char avc_seq_buf[4096];
	char* pbuf = avc_seq_buf;

	unsigned char flag = 0;
	flag = (1 << 4) |   // frametype "1  == keyframe"
		7;              // codecid   "7  == AVC"

	pbuf = UI08ToBytes(pbuf, flag);

	pbuf = UI08ToBytes(pbuf, 0);    // avc packet type (0, header)
	pbuf = UI24ToBytes(pbuf, 0);    // composition time

	// AVCDecoderConfigurationRecord

	pbuf = UI08ToBytes(pbuf, 1);            // configurationVersion
	pbuf = UI08ToBytes(pbuf, spsBuf[1]);      // AVCProfileIndication
	pbuf = UI08ToBytes(pbuf, spsBuf[2]);      // profile_compatibility
	pbuf = UI08ToBytes(pbuf, spsBuf[3]);      // AVCLevelIndication
	pbuf = UI08ToBytes(pbuf, 0xff);         // 6 bits reserved (111111) + 2 bits nal size length - 1
	pbuf = UI08ToBytes(pbuf, 0xe1);         // 3 bits reserved (111) + 5 bits number of sps (00001)
	pbuf = UI16ToBytes(pbuf, (unsigned short)spsSize);    // sps
	memcpy(pbuf, spsBuf, spsSize);
	pbuf += spsSize;
	pbuf = UI08ToBytes(pbuf, 1);            // number of pps
	pbuf = UI16ToBytes(pbuf, (unsigned short)ppsSize);    // pps
	memcpy(pbuf, ppsBuf, ppsSize);
	pbuf += ppsSize;

	return LibRtmpSendData(avc_seq_buf, (int)(pbuf - avc_seq_buf), 
		FLV_TAG_TYPE_VIDEO, 0);
}

int LibRtmpServer::LibRtmpSendAACHeader(int sampleRate, int channel)
{
	char aac_seq_buf[4096];
	char* pbuf = aac_seq_buf;
	PutBitContext pb;
	int sample_rate_index = 0x04;

	unsigned char flag = 0;
	flag = (10 << 4) |  // soundformat "10 == AAC"
		(3 << 2) |      // soundrate   "3  == 44-kHZ"
		(1 << 1) |      // soundsize   "1  == 16bit"
		1;              // soundtype   "1  == Stereo"

	pbuf = UI08ToBytes(pbuf, flag);

	pbuf = UI08ToBytes(pbuf, 0);    // aac packet type (0, header)

	// AudioSpecificConfig

	if (sampleRate == 48000)        sample_rate_index = 0x03;
	else if (sampleRate == 44100)   sample_rate_index = 0x04;
	else if (sampleRate == 32000)   sample_rate_index = 0x05;
	else if (sampleRate == 24000)   sample_rate_index = 0x06;
	else if (sampleRate == 22050)   sample_rate_index = 0x07;
	else if (sampleRate == 16000)   sample_rate_index = 0x08;
	else if (sampleRate == 12000)   sample_rate_index = 0x09;
	else if (sampleRate == 11025)   sample_rate_index = 0x0a;
	else if (sampleRate == 8000)    sample_rate_index = 0x0b;


	init_put_bits(&pb, pbuf, 1024);
	put_bits(&pb, 5, 2);    //object type - AAC-LC
	put_bits(&pb, 4, sample_rate_index); // sample rate index
	put_bits(&pb, 4, channel);    // channel configuration
	//GASpecificConfig
	put_bits(&pb, 1, 0);    // frame length - 1024 samples
	put_bits(&pb, 1, 0);    // does not depend on core coder
	put_bits(&pb, 1, 0);    // is not extension

	flush_put_bits(&pb);

	pbuf += 2;

	return LibRtmpSendData(aac_seq_buf, (int)(pbuf - aac_seq_buf), 
		FLV_TAG_TYPE_AUDIO, 0);
}

void LibRtmpServer::LibRtmpResetTimestamp()
{
	time_begin_ = GetCurrentTickCount();
}

unsigned int LibRtmpServer::LibRtmpGetTimestamp()
{
	unsigned int timestamp;
	long long now = GetCurrentTickCount();

	//if (now < last_timestamp_)
	if (now < time_begin_)
	{
		timestamp = 0;
		//last_timestamp_ = now;
		time_begin_ = now;
	}
	else
	{
		timestamp = (unsigned int)(now -  time_begin_);
		//timestamp = now - last_timestamp_;
	}
	return timestamp;
}

//////////////////////////////////////////////////////////////////////////

int LibRtmpServer::SendConnectResult(double txn)
{
	RTMPPacket packet;
	char pbuf[384], *pend = pbuf+sizeof(pbuf);
	AMFObject obj;
	AMFObjectProperty p, op;
	AVal av;
	char *enc = 0;

	packet.m_nChannel = 0x03;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av__result);
	enc = AMF_EncodeNumber(enc, pend, txn);
	*enc++ = AMF_OBJECT;

	STR2AVAL(av, "FMS/3,5,1,525");
	enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_mode, 1.0);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	*enc++ = AMF_OBJECT;

	STR2AVAL(av, "status");
	enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
	STR2AVAL(av, "NetConnection.Connect.Success");
	enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
	STR2AVAL(av, "Connection succeeded.");
	enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, rtmp->m_fEncoding);
#if 0
	STR2AVAL(av, "58656322c972d6cdf2d776167575045f8484ea888e31c086f7b5ffbd0baec55ce442c2fb");
	enc = AMF_EncodeNamedString(enc, pend, &av_secureToken, &av);
#endif
	STR2AVAL(p.p_name, "version");
	STR2AVAL(p.p_vu.p_aval, "3,5,1,525");
	p.p_type = AMF_STRING;
	obj.o_num = 1;
	obj.o_props = &p;
	op.p_type = AMF_OBJECT;
	STR2AVAL(op.p_name, "data");
	op.p_vu.p_object = obj;
	enc = AMFProp_Encode(&op, enc, pend);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	packet.m_nBodySize = enc - packet.m_body;

	return RTMP_SendPacket(rtmp, &packet, FALSE);
}

int LibRtmpServer::SendResultNumber(double txn, double ID)
{
	RTMPPacket packet;
	char pbuf[256], *pend = pbuf+sizeof(pbuf);
	char *enc = 0;

	packet.m_nChannel = 0x03;     // control channel (invoke)
	packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
	packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = 0;
	packet.m_hasAbsTimestamp = 0;
	packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

	enc = packet.m_body;
	enc = AMF_EncodeString(enc, pend, &av__result);
	enc = AMF_EncodeNumber(enc, pend, txn);
	*enc++ = AMF_NULL;
	enc = AMF_EncodeNumber(enc, pend, ID);

	packet.m_nBodySize = enc - packet.m_body;

	return RTMP_SendPacket(rtmp, &packet, FALSE);
}

int LibRtmpServer::ServeInvoke(RTMPPacket *packet, unsigned int offset)
{
	const char *body;
	unsigned int nBodySize;
	int ret = 0, nRes;

	AMFObject obj;
	AVal method;
	double txn = 0;

	body = packet->m_body + offset;
	nBodySize = packet->m_nBodySize - offset;

	if (body[0] != 0x02)		// make sure it is a string method name we start with
	{
		RTMP_Log(RTMP_LOGWARNING, "%s, Sanity failed. no string method in invoke packet",
			__FUNCTION__);
		return 0;
	}

	nRes = AMF_Decode(&obj, body, nBodySize, FALSE);
	if (nRes < 0)
	{
		RTMP_Log(RTMP_LOGERROR, "%s, error decoding invoke packet", __FUNCTION__);
		return 0;
	}

	AMF_Dump(&obj);
	AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);
	txn = AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));
	RTMP_Log(RTMP_LOGDEBUG, "%s, client invoking <%s>", __FUNCTION__, method.av_val);

	if (AVMATCH(&method, &av_connect))
	{
		AMFObject cobj;
		AVal pname, pval;
		int i;

		//connect = packet->m_body;
		//packet->m_body = NULL;

		AMFProp_GetObject(AMF_GetProp(&obj, NULL, 2), &cobj);
		for (i=0; i<cobj.o_num; i++)
		{
			pname = cobj.o_props[i].p_name;
			pval.av_val = NULL;
			pval.av_len = 0;
			if (cobj.o_props[i].p_type == AMF_STRING)
				pval = cobj.o_props[i].p_vu.p_aval;
			if (AVMATCH(&pname, &av_app))
			{
				rtmp->Link.app = pval;
				pval.av_val = NULL;
				if (!rtmp->Link.app.av_val)
					rtmp->Link.app.av_val = "";
			}
			else if (AVMATCH(&pname, &av_flashVer))
			{
				rtmp->Link.flashVer = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_swfUrl))
			{
				rtmp->Link.swfUrl = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_tcUrl))
			{
				rtmp->Link.tcUrl = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_pageUrl))
			{
				rtmp->Link.pageUrl = pval;
				pval.av_val = NULL;
			}
			else if (AVMATCH(&pname, &av_audioCodecs))
			{
				rtmp->m_fAudioCodecs = cobj.o_props[i].p_vu.p_number;
			}
			else if (AVMATCH(&pname, &av_videoCodecs))
			{
				rtmp->m_fVideoCodecs = cobj.o_props[i].p_vu.p_number;
			}
			else if (AVMATCH(&pname, &av_objectEncoding))
			{
				rtmp->m_fEncoding = cobj.o_props[i].p_vu.p_number;
			}
		}
		if (obj.o_num > 3)
		{
			int i = obj.o_num - 3;
			rtmp->Link.extras.o_num = i;
			rtmp->Link.extras.o_props = (AMFObjectProperty*)malloc(i*sizeof(AMFObjectProperty));
			memcpy(rtmp->Link.extras.o_props, obj.o_props+3, i*sizeof(AMFObjectProperty));
			obj.o_num = 3;
		}
		RTMP_SendCtrl(rtmp, 0, 0, 0);
		SendConnectResult(txn);
	}
	else if (AVMATCH(&method, &av_createStream))
	{
		SendResultNumber(txn, ++rtmp->m_stream_id);
	}
	else if (AVMATCH(&method, &av_getStreamLength))
	{
		SendResultNumber(txn, 10.0);
	}
	else if (AVMATCH(&method, &av_NetStream_Authenticate_UsherToken))
	{
		AVal usherToken;
		AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &usherToken);
		AVreplace(&usherToken, &av_dquote, &av_escdquote);
		rtmp->Link.usherToken = usherToken;
	}
	else if (AVMATCH(&method, &av_play))
	{
		RTMPPacket pc = {0};

		AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &rtmp->Link.playpath);

		//pc.m_body = connect;
		//connect = NULL;
		//RTMPPacket_Free(&pc);
		//ret = 1;

		
		m_pown->RtmpConnection_OnClientPlay(rtmp->Link.playpath.av_val);
	}
	else if (AVMATCH(&method, &av_pause))
	{
		
		m_pown->RtmpConnection_OnClientPause();
	}
	AMF_Reset(&obj);
	return ret;
}

int LibRtmpServer::DispatchRtmpPacket(RTMPPacket *packet)
{
	int ret = 0;

	RTMP_Log(RTMP_LOGDEBUG, "%s, received packet type %02X, size %u bytes", __FUNCTION__,
		packet->m_packetType, packet->m_nBodySize);

	switch (packet->m_packetType)
	{
	case 0x01:  // chunk size
		break;
	case 0x03:  // bytes read report
		break;
	case 0x04:  // ctrl
		break;
	case 0x05:  // server bw
		break;
	case 0x06:  // client bw
		break;
	case 0x08:  // audio data
		break;
	case 0x09:  // video data
		break;
	case 0x0F:  // flex stream send
		break;
	case 0x10:  // flex shared object
		break;
	case 0x11:  // flex message
		{
			RTMP_Log(RTMP_LOGDEBUG, "%s, flex message, size %u bytes, not fully supported",
				__FUNCTION__, packet->m_nBodySize);
			if (ServeInvoke(packet, 1))
				RTMP_Close(rtmp);
		}
		break;
	case 0x12:  // metadata (notify)
		break;
	case 0x13:  // shared object
		break;
	case 0x14:  // invoke
		{
			RTMP_Log(RTMP_LOGDEBUG, "%s, received: invoke %u bytes", __FUNCTION__,
				packet->m_nBodySize);
			RTMP_LogHex(RTMP_LOGDEBUG, (const uint8_t*)packet->m_body, packet->m_nBodySize);
			if (ServeInvoke(packet, 0))
				RTMP_Close(rtmp);
		}
		break;

	case 0x16:  // flv
		break;
	default:
		RTMP_Log(RTMP_LOGDEBUG, "%s, unknown packet type received: 0x%02x", __FUNCTION__,
			packet->m_packetType);
#ifdef _DEBUG
		RTMP_LogHex(RTMP_LOGDEBUG, (const uint8_t*)packet->m_body, packet->m_nBodySize);
#endif
	}
	return ret;
}

void AVreplace(AVal *src, const AVal *orig, const AVal *repl)
{
	char *srcbeg = src->av_val;
	char *srcend = src->av_val + src->av_len;
	char *dest, *sptr, *dptr;
	int n = 0;

	/* count occurrences of orig in src */
	sptr = src->av_val;
	while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
	{
		n++;
		sptr += orig->av_len;
	}
	if (!n)
		return;

	dest = (char *)malloc(src->av_len + 1 + (repl->av_len - orig->av_len) * n);

	sptr = src->av_val;
	dptr = dest;
	while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
	{
		n = sptr - srcbeg;
		memcpy(dptr, srcbeg, n);
		dptr += n;
		memcpy(dptr, repl->av_val, repl->av_len);
		dptr += repl->av_len;
		sptr += orig->av_len;
		srcbeg = sptr;
	}
	n = srcend - srcbeg;
	memcpy(dptr, srcbeg, n);
	dptr += n;
	*dptr = '\0';
	src->av_val = dest;
	src->av_len = dptr - dest;
}
