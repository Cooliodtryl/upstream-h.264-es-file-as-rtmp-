#include "aac_frame_reader.h"

#include <stdlib.h>
#include <string.h>

#include "rtmp_server_base/bit_reader.h"
#include "rtmp_server_base/thread.h"
#include "rtmp_server_base/util_tools.h"

void AACFrameReader_Init(AACFrameReader* frameReader, const char* filename, int isRepeat)
{
    FILE* fp = fopen(filename, "rb");
    frameReader->is_repeat_ = isRepeat;
    frameReader->filebuf_ = 0;
    frameReader->filesize_ = 0;

    if (fp)
    {
        int retval = 0;
        fseek(fp, 0, SEEK_END);
        frameReader->filesize_ = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        frameReader->filebuf_ = (char*)malloc(frameReader->filesize_);
        retval = fread(frameReader->filebuf_, 1, frameReader->filesize_, fp);

        fclose(fp);
    }
    frameReader->pbuf_ = frameReader->filebuf_;

    frameReader->is_stop_ = 0;
    frameReader->caller_ = NULL;
    frameReader->data_callback_ = NULL;
}

void AACFrameReader_Free(AACFrameReader* frameReader)
{
    free(frameReader->filebuf_);
}

int AACFrameReader_Parse(AACFrameReader* frameReader, int* samRate, int* channel)
{
    const char *p = 0;
    const char *end = 0;

    BitReader bit_reader;

    p = frameReader->pbuf_;
    end = frameReader->filebuf_ + frameReader->filesize_;

    BitReader_Init(&bit_reader, p);
    while (BitReader_ShowBitUI32(&bit_reader, 12) != 0xfff)
    {
        BitReader_SkipBytes(&bit_reader, 1);
    }

    frameReader->pbuf_ = BitReader_SrcBuf(&bit_reader) + BitReader_BytePos(&bit_reader);

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

int AACFrameReader_ReadFrame(AACFrameReader* frameReader, char* outBuf, 
    int* outBufSize)
{
    const char *p = 0;
    const char *end = 0;

    BitReader bit_reader;
    int framelen = 0;

    if (frameReader->pbuf_ >= frameReader->filebuf_ + frameReader->filesize_)
    {
        if (frameReader->is_repeat_)
        {
            frameReader->pbuf_ = frameReader->filebuf_;
        }
        else
        {
            return 0;
        }
    }

    p = frameReader->pbuf_;
    end = frameReader->filebuf_ + frameReader->filesize_;

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
    frameReader->pbuf_ = p;

    return 1;
}

static const int kSampleRateIndexs[16] = {
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000, 7350
};

TFTYPE AACFrameReader_Thread(void* arg)
{
    AACFrameReader* frameReader = (AACFrameReader*)arg;

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
        AACFrameReader_Parse(frameReader, &sam_rate_index, &channel);
        sample_rate = kSampleRateIndexs[sam_rate_index];
        frame_interval = 1000.0 / (sample_rate / 1024.0);
    }

    while (!frameReader->is_stop_)
    {
        if (AACFrameReader_ReadFrame(frameReader, tmpbuf, &tmpbuf_len))
        {
            // 接收到aac裸流, 保存当前帧
            if (frameReader->data_callback_ != NULL)
            {
                frameReader->data_callback_(frameReader->caller_,
                    tmpbuf, tmpbuf_len);
            }
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

    frameReader->pbuf_ = 0;

    TFRET();
}

void AACFrameReader_Start(AACFrameReader* frameReader)
{
    frameReader->is_stop_ = 0;
    ThreadCreate(AACFrameReader_Thread, frameReader);
}

void AACFrameReader_Stop(AACFrameReader* frameReader)
{
    frameReader->is_stop_ = 1;
    for (;;)
    {
        if (frameReader->pbuf_ == NULL)
            break;

        MySleep(100);
    }
}

void AACFrameReader_SetCallback(AACFrameReader* frameReader,
    OnAACDataCallback callback, void* caller)
{
    frameReader->caller_ = caller;
    frameReader->data_callback_ = callback;
}
