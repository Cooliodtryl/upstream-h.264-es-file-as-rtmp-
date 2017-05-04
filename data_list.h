#ifndef _DATA_LIST_H_
#define _DATA_LIST_H_

#include "rtmp_server_base/thread.h"

typedef struct RefCountBuf
{
    char* data_;
    int data_len_;
    atomic_ref_t ref_count_;
} RefCountBuf;

RefCountBuf* RefCountBuf_New(int bufSize);

void RefCountBuf_Delete(RefCountBuf* refbuf);

RefCountBuf* RefCountBuf_Copy(RefCountBuf* srcRefbuf);

typedef struct DataListNode
{
    struct DataListNode* prev_;
    struct DataListNode* next_;

    RefCountBuf* buf_;

    int is_video_;
    int is_keyframe_;
} DataListNode;

DataListNode* DataListNode_Create(RefCountBuf* refbuf, 
    int isVideo, int isKeyframe);

void DataListNode_Destroy(DataListNode* node);

void DataListNode_Remove(DataListNode* node);

void DataListNode_InsertBefore(DataListNode* thisNode, DataListNode* someNode);

void DataListNode_InsertAfter(DataListNode* thisNode, DataListNode* someNode);

typedef struct DataList
{
    DataListNode* root_;
    int size_;
} DataList;

DataList* DataList_Create();

void DataList_Destroy(DataList* dataList);

void DataList_Pushback(DataList* dataList, RefCountBuf* refbuf,
    int isVideo, int isKeyframe);

void DataList_Popfront(DataList* dataList);

void DataList_AppendNode(DataList* dataList, DataListNode* node);

DataListNode* DataList_Head(DataList* dataList);

DataListNode* DataList_Tail(DataList* dataList);

DataListNode* DataList_End(DataList* dataList);

int DataList_IsEmpty(DataList* dataList);

void DataList_Swap(DataList* fromList, DataList* toList);

#endif // _DATA_LIST_H_
