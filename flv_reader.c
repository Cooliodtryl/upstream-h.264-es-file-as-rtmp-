#include "flv_reader.h"

void FlvReaderOpen(FlvReader* flvReader, const char* filename)
{
    FlvFileHeader flvheader;
     unsigned int headersize = 0;

    flvReader->fp_ = fopen(filename, "rb");

    fseek(flvReader->fp_, 0, SEEK_END);
    flvReader->filesize_ = ftell(flvReader->fp_);
    fseek(flvReader->fp_, 0, SEEK_SET);

    // 读取flv文件头
   
    fread(&flvheader, sizeof(FlvFileHeader), 1, flvReader->fp_);

    // 跳过flv头剩余的大小
    headersize = FLV_UI32(flvheader.headersize);
    fseek(flvReader->fp_, headersize-sizeof(FlvFileHeader), SEEK_CUR);

    // 跳过第一个PreviousTagSize
    fseek(flvReader->fp_, 4, SEEK_CUR);

    flvReader->read_pos_ = headersize + 4;
}

void FlvReaderClose(FlvReader* flvReader)
{
    fclose(flvReader->fp_);
}

void FlvReaderRewind(FlvReader* flvReader)
{
    FlvFileHeader flvheader;
    unsigned int headersize = 0;

    fseek(flvReader->fp_, 0, SEEK_SET);
    // 读取flv文件头

    fread(&flvheader, sizeof(FlvFileHeader), 1, flvReader->fp_);

    // 跳过flv头剩余的大小
    headersize = FLV_UI32(flvheader.headersize);
    fseek(flvReader->fp_, headersize-sizeof(FlvFileHeader), SEEK_CUR);

    // 跳过第一个PreviousTagSize
    fseek(flvReader->fp_, 4, SEEK_CUR);

    flvReader->read_pos_ = headersize + 4;
}

int FlvReaderReadNextTagHeader(FlvReader* flvReader, int* tagType, int* timestamp)
{
    FlvTag flvtag;

    if (flvReader->read_pos_ + sizeof(FlvTag) > flvReader->filesize_)
    {
        return -1;
    }

    fread(&flvtag, sizeof(FlvTag), 1, flvReader->fp_);
    flvReader->read_pos_ += sizeof(FlvTag);

    *timestamp = (flvtag.timestamp_ex << 24) + 
        (flvtag.timestamp[0] << 16) + 
        (flvtag.timestamp[1] << 8) + 
        (flvtag.timestamp[2]);
    *tagType = flvtag.type;
    flvReader->reading_tag_data_size_ = FLV_UI24(flvtag.datasize);

    return flvReader->reading_tag_data_size_;
}

void FlvReaderReadNextTagData(FlvReader* flvReader, char* outBuf)
{
    int readsize = fread(outBuf, 1, flvReader->reading_tag_data_size_, flvReader->fp_);
    flvReader->read_pos_ += flvReader->reading_tag_data_size_;

    fseek(flvReader->fp_, 4, SEEK_CUR);
    flvReader->read_pos_ += 4;
}
