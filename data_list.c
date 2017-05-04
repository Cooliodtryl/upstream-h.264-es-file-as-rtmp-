#include "data_list.h"

#include <stdlib.h>
#include <memory.h>

RefCountBuf* RefCountBuf_New(int bufSize)
{
    RefCountBuf* refbuf = (RefCountBuf*)malloc(sizeof(RefCountBuf));
    refbuf->data_ = NULL;
    refbuf->data_len_ = 0;
    if (bufSize > 0)
    {
        refbuf->data_ = (char*)malloc(bufSize);
        refbuf->data_len_ = bufSize;
    }
    AtomicInit(&refbuf->ref_count_);
    AtomicIncRef(&refbuf->ref_count_);
    return refbuf;
}

void RefCountBuf_Delete(RefCountBuf* refbuf)
{
    if (refbuf == NULL) return;
    if (AtomicDecRef(&refbuf->ref_count_) == 0)
    {
        if (refbuf->data_)
        {
            free(refbuf->data_);
            refbuf->data_ = NULL;
            refbuf->data_len_ = 0;
        }
        AtomicRelease(&refbuf->ref_count_);
        free(refbuf);
    }
}

RefCountBuf* RefCountBuf_Copy(RefCountBuf* srcRefbuf)
{
    if (srcRefbuf == NULL) return NULL;

    AtomicIncRef(&srcRefbuf->ref_count_);
    return srcRefbuf;
}

DataListNode* DataListNode_Create(RefCountBuf* refbuf,
    int isVideo, int isKeyframe)
{
    DataListNode* node = (DataListNode*)malloc(sizeof(DataListNode));

    node->prev_ = NULL;
    node->next_ = NULL;
    node->buf_ = RefCountBuf_Copy(refbuf);
    node->is_video_ = isVideo;
    node->is_keyframe_ = isKeyframe;

    return node;
}

void DataListNode_Destroy(DataListNode* node)
{
    RefCountBuf_Delete(node->buf_);

    free(node);
}

void DataListNode_Remove(DataListNode* node)
{
    node->prev_->next_ = node->next_;
    node->next_->prev_ = node->prev_;
}

void DataListNode_InsertBefore(DataListNode* thisNode, DataListNode* someNode)
{
    thisNode->next_ = someNode;
    thisNode->prev_ = someNode->prev_;
    someNode->prev_->next_ = thisNode;
    someNode->prev_ = thisNode;
}

void DataListNode_InsertAfter(DataListNode* thisNode, DataListNode* someNode)
{
    thisNode->next_ = someNode->next_;
    thisNode->prev_ = someNode;
    someNode->next_->prev_ = thisNode;
    someNode->next_ = thisNode;
}

DataList* DataList_Create()
{
    DataList* data_list = (DataList*)malloc(sizeof(DataList));
    data_list->root_ = DataListNode_Create(NULL, 0, 0);
    data_list->root_->prev_ = data_list->root_;
    data_list->root_->next_ = data_list->root_;
    data_list->size_ = 0;
    return data_list;
}

void DataList_Destroy(DataList* dataList)
{
    DataListNode* node;
    for (node = DataList_Head(dataList); node != DataList_End(dataList);)
    {
        DataListNode* thisnode = node;
        node = node->next_;

        DataListNode_Destroy(thisnode);
    }
    DataListNode_Destroy(dataList->root_);
    free(dataList);
}

void DataList_Pushback(DataList* dataList, RefCountBuf* refbuf,
    int isVideo, int isKeyframe)
{
    DataListNode* node = DataListNode_Create(refbuf, isVideo, isKeyframe);
    DataListNode_InsertBefore(node, dataList->root_);
    dataList->size_++;
}

void DataList_Popfront(DataList* dataList)
{
    DataListNode_Remove(DataList_Head(dataList));
    dataList->size_--;
}

void DataList_AppendNode(DataList* dataList, DataListNode* node)
{
    DataListNode_InsertBefore(node, dataList->root_);
    dataList->size_++;
}

DataListNode* DataList_Head(DataList* dataList)
{
    return dataList->root_->next_;
}

DataListNode* DataList_Tail(DataList* dataList)
{
    return dataList->root_->prev_;
}

DataListNode* DataList_End(DataList* dataList)
{
    return dataList->root_;
}

int DataList_IsEmpty(DataList* dataList)
{
    if (DataList_Head(dataList) == DataList_End(dataList))
    {
        return 1;
    }
    return 0;
}

void DataList_Swap(DataList* fromList, DataList* toList)
{
    DataListNode* tmp = fromList->root_;
    fromList->root_ = toList->root_;
    toList->root_ = tmp;
}
