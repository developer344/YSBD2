#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "HT.h"
#include "BF.h"

int HT_CreateIndex(char *fileName, char attrType, char *attrName, int attrLength, int buckets)
{
    HT_info *info = malloc(sizeof(HT_info));
    if (BF_CreateFile(fileName) < 0)
    {
        BF_PrintError("Error creating file");
        return -1;
    }
    if ((info->fileDesc = BF_OpenFile(fileName)) < 0)
    {
        BF_PrintError("Error opening file\n");
        return -1;
    }
    if (BF_AllocateBlock(info->fileDesc) < 0)
    {
        BF_PrintError("Error allocating block");
        return -1;
    }
    void *blockh;
    if (BF_ReadBlock(info->fileDesc, 0, &blockh) < 0)
    {
        BF_PrintError("Error reading block");
        return -1;
    }
    //Initialisation of HT_info
    info->attrLength = attrLength;
    info->attrType = attrType;
    info->attrName = malloc(sizeof(char) * info->attrLength);
    strncpy(info->attrName, attrName, attrLength);
    info->numBuckets = buckets;
    info->firstHeaderBlock = 1;
    //First block is the first header info block and only has HP info
    memcpy(blockh, info, sizeof(HT_info));
    free(info);
    if (BF_WriteBlock(info->fileDesc, 0))
    {
        BF_PrintError("Error writting block");
        return -1;
    }
    if (BF_AllocateBlock(info->fileDesc) < 0)
    {
        BF_PrintError("Error allocating block");
        return -1;
    }
    void *block;
    if (BF_ReadBlock(info->fileDesc, BF_GetBlockCounter(info->fileDesc) - 1, &block) < 0)
    {
        BF_PrintError("Error reading block");
        return -1;
    }
    //Creating frst header block
    //First header block and the other header blocks have the number of the block
    //that corresponds to the buckets.

    //Every header block has the bytes left in the block,
    //the pisition of the next header block and the pisitions of at most 126 buckets.
    HeaderInfo *h_info = malloc(sizeof(HeaderInfo));
    h_info->bytesLeft = BLOCK_SIZE - sizeof(HeaderInfo);
    h_info->nextHeaderBlock = -1;
    memcpy(block, h_info, sizeof(HeaderInfo));
    free(h_info);
    if (BF_WriteBlock(info->fileDesc, BF_GetBlockCounter(info->fileDesc) - 1) < 0)
    {
        BF_PrintError("Error writting block");
        return -1;
    }
    int HB_offset = 0;
    int headerblockindex = BF_GetBlockCounter(info->fileDesc) - 1; // block
    int bucketposition;
    for (int i = 0; i < buckets; i++)
    {
        //Allocating bucket blocks
        if (BF_AllocateBlock(info->fileDesc) < 0)
        {
            BF_PrintError("Error allocating block");
            return -1;
        }
        if (BF_ReadBlock(info->fileDesc, (bucketposition = BF_GetBlockCounter(info->fileDesc) - 1), &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        //Initializing bucket blocks
        BucketInfo *b_info = malloc(sizeof(BucketInfo));
        b_info->numOfRec = 0;
        b_info->nextBlock = -1;
        b_info->key = i;
        b_info->bytesLeft = BLOCK_SIZE - sizeof(BucketInfo);
        memcpy(block, b_info, sizeof(BucketInfo));
        if (BF_WriteBlock(info->fileDesc, bucketposition) < 0)
        {
            BF_PrintError("Error writting block");
            return -1;
        }
        if (BF_ReadBlock(info->fileDesc, headerblockindex, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        //If the header block is full I create a new one to contue referencing the header blocks
        HeaderInfo *head_info = block;
        if (head_info->bytesLeft < sizeof(int)) //bytesleft
        {
            if (BF_AllocateBlock(info->fileDesc) < 0)
            {
                BF_PrintError("Error allocating block");
                return -1;
            }
            //Allocating new header block putting its position to the variable (nextHeaderBlock)
            //of the previous header block
            int nextheaderblock = BF_GetBlockCounter(info->fileDesc) - 1;
            head_info->nextHeaderBlock = nextheaderblock;
            memcpy(block, head_info, sizeof(HeaderInfo));
            if (BF_WriteBlock(info->fileDesc, headerblockindex))
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            headerblockindex = nextheaderblock;
            if (BF_ReadBlock(info->fileDesc, headerblockindex, &block) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            //Initializing new header block
            h_info = malloc(sizeof(HeaderInfo));
            h_info->bytesLeft = BLOCK_SIZE - sizeof(HeaderInfo);
            h_info->nextHeaderBlock = -1;
            memcpy(block, h_info, sizeof(HeaderInfo));
            free(h_info);
            HB_offset = 0;
        }
        //Puttting the position of the last allocated bucket t the currrent headerblock so I can reference it later
        head_info = block;
        memcpy(block + sizeof(HeaderInfo) + HB_offset * sizeof(int), &bucketposition, sizeof(int));
        head_info->bytesLeft -= sizeof(int);
        memcpy(block, head_info, sizeof(HeaderInfo));
        HB_offset++;
        if (BF_WriteBlock(info->fileDesc, headerblockindex))
        {
            BF_PrintError("Error writting block");
            return -1;
        }
    }
    //Closing index
    if (BF_CloseFile(info->fileDesc) < 0)
    {
        BF_PrintError("Error closing file");
        return -1;
    }
    return 0;
}

HT_info *HT_OpenIndex(char *fileName)
{
    int fDesc;
    //Opening file and returning header info
    if ((fDesc = BF_OpenFile(fileName)) < 0)
    {
        BF_PrintError("Error opening file");
        return NULL;
    }
    void *block;
    if (BF_ReadBlock(fDesc, 0, &block) < 0)
    {
        BF_PrintError("Error reading block");
        return NULL;
    }
    HT_info *returnValue = malloc(sizeof(HT_info));
    memcpy(returnValue, block, sizeof(HT_info));
    if (BF_WriteBlock(fDesc, 0))
    {
        BF_PrintError("Error writting block");
        return NULL;
    }
    return returnValue;
}

int HT_CloseIndex(HT_info *header_info)
{
    //Closing file
    if (BF_CloseFile(header_info->fileDesc) < 0)
    {
        BF_PrintError("Error closing file");
        return -1;
    }
    free(header_info);
    return 0;
}

int HT_InsertEntry(HT_info header_info, Record record)
{

    //I check to see if Record with the same id already exists
    if (HT_GetAllEntries(header_info, (void *)(&record.id)) >= 0)
    {
        printf("Error: record with the same id is already in the file\n");
        return -1;
    }
    //Calculating the key
    int key = record.id % header_info.numBuckets;
    // 126 is the max number of bucket positions a header block can store
    //Calculating the header block where the position of bucket with that key is located
    int headerblock = key / 126;
    //Calculating the position in the header block where the bucket position is located
    int headerindex = key % 126;
    int bucketindex = -1;
    void *blockh;
    //Opening the first header block whitch is in position 1
    int index = 1;
    int i = 0;
    int next = 1;
    HeaderInfo *h_info;
    //Searching for the header block
    do
    {
        if (BF_ReadBlock(header_info.fileDesc, next, &blockh) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        h_info = blockh;
        if (i == headerblock)
        {
            //Once I find the header block I store the position of the bucket that correspondes to the key I ccalculated
            memcpy(&bucketindex, blockh + sizeof(HeaderInfo) + (headerindex * (int)sizeof(int)), sizeof(int));
            break;
        }
        if (BF_WriteBlock(header_info.fileDesc, next) < 0)
        {

            BF_PrintError("Error writting block");
            return -1;
        }
        next = h_info->nextHeaderBlock;
        i++;

    } while (next != -1);
    next = bucketindex;
    BucketInfo *b_info;
    //I search in the bucket for a block that has space for the record I want to insert
    do
    {
        void *blockb;
        bucketindex = next;

        if (BF_ReadBlock(header_info.fileDesc, bucketindex, &blockb) < 0)
        {

            BF_PrintError("Error reading block");
            return -1;
        }
        b_info = blockb;
        next = b_info->nextBlock;
        //Each block can store up to 4 records so that it can be at most 80% full
        if (b_info->numOfRec < 4)
        {
            //If I find a block with space I store te record to the last position of the block

            //Storing record
            memcpy(blockb + sizeof(BucketInfo) + b_info->numOfRec * (int)sizeof(Record), &record, sizeof(Record));
            b_info->numOfRec += 1;
            b_info->bytesLeft -= sizeof(Record);
            memcpy(blockb, b_info, sizeof(BucketInfo));
            //Storing the new information for the block
            if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            printf("\nRecord with id: %d, name: %s, surname: %s, address: %s\n", record.id, record.name, record.surname, record.address);
            printf("Added to Block: %d\n\n", bucketindex);
            //returning the position of the block that it was stored in
            return bucketindex;
        }
        else if (next == -1)
        {
            //If I don't find space in the previously allocated blocks of the bucket I allocate a new one
            //and store the record there

            //Allocating new block
            if (BF_AllocateBlock(header_info.fileDesc) < 0)
            {
                BF_PrintError("Error allocating block");
                return -1;
            }
            void *blockbucket;
            index = BF_GetBlockCounter(header_info.fileDesc) - 1;
            if (BF_ReadBlock(header_info.fileDesc, index, &blockbucket) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            //Initializing new block
            BucketInfo *b_info_new = malloc(sizeof(BucketInfo));
            b_info_new->key = key;
            b_info_new->bytesLeft = BLOCK_SIZE - sizeof(BucketInfo) - sizeof(Record);
            b_info_new->numOfRec = 1;
            b_info_new->nextBlock = -1;
            memcpy(blockbucket, b_info_new, sizeof(BucketInfo));
            //Storing the record to the first position of the block
            memcpy(blockbucket + sizeof(BucketInfo), &record, sizeof(Record));

            if (BF_WriteBlock(header_info.fileDesc, index) < 0)
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            //Storing the position of the newly allocated block to the previous block
            b_info->nextBlock = index;
            memcpy(blockb, b_info, sizeof(BucketInfo));
            if (BF_WriteBlock(header_info.fileDesc, bucketindex))
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            printf("\nRecord with id: %d, name: %s, surname: %s, address: %s\n", record.id, record.name, record.surname, record.address);
            printf("Added to Block: %d\n\n", BF_GetBlockCounter(header_info.fileDesc) - 1);
            //returning the position of the block that it was stored in
            return BF_GetBlockCounter(header_info.fileDesc) - 1;
        }

        if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
        {
            BF_PrintError("Error writting block");
            return -1;
        };
        //I repeate this until all blocks of the bucket are traversed (if full) so as to allocate the new block
    } while (1);
    return -1;
}

int HT_DeleteEntry(HT_info header_info, void *value)
{
    int bucketindex;
    //Checking if there is a record in the file with such value so that it can be deleted
    if ((bucketindex = HT_GetAllEntries(header_info, value)) < 0)
    {
        printf("There is no record with value: %d\n", *((int *)value));

        return -1;
    }
    //If the record with the value given is in one of the blocks then HT_GetAllEntries  returns the
    //position of the block that it is in so it can be deleted

    //Reading the block given by HT_GetAllEntries
    void *block;
    if (BF_ReadBlock(header_info.fileDesc, bucketindex, &block) < 0)
    {
        BF_PrintError("Error reading block");

        return -1;
    }
    BucketInfo *b_info = block;
    int i;
    //Searching the record in the block
    for (i = 0; i < b_info->numOfRec; i++)
    {
        Record *record = block + sizeof(BucketInfo) + (i * sizeof(Record));
        if (record->id == *((int *)value))
        {
            //When I find it I check if it is located at the last position of the block
            if (i == b_info->numOfRec - 1)
            {
                //If it is I just decrease the amount of blocks in the block and I increase the amount of
                //bytes left by the size of the record
                b_info->numOfRec -= 1;
                b_info->bytesLeft += (int)sizeof(Record);
                //Copy the info back to the block
                memcpy(block, b_info, sizeof(BucketInfo));
                if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
                {
                    BF_PrintError("Error writting block");

                    return -1;
                }
                //Return 0 for success
                return 0;
            }
            else
            {
                //If its not the last record in the block I take the last record and put it in its place
                Record *lastRecord = block + sizeof(BucketInfo) + ((b_info->numOfRec - 1) * sizeof(Record));
                memcpy(block + sizeof(BucketInfo) + (i * sizeof(Record)), lastRecord, sizeof(Record));
                //Updating inforation
                b_info->numOfRec -= 1;
                b_info->bytesLeft += sizeof(Record);
                //Storing back inforamtion
                memcpy(block, b_info, sizeof(BucketInfo));
                if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
                {
                    BF_PrintError("Error writting block");

                    return -1;
                }
                //Return 0 for success
                return 0;
            }
        }
    }
    if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
    {
        BF_PrintError("Error writting block");

        return -1;
    }
    //Return -1 for failure
    return -1;
}

int HT_GetAllEntries(HT_info header_info, void *value)
{
    //Calculating the key
    int key = *((int *)value) % (int)header_info.numBuckets;
    // 126 is the max number of bucket positions a header block can store
    //Calculating the header block where the position of bucket with that key is located
    int headerblock = key / 126;
    //Calculating the position in the header block where the bucket position is located
    int headerindex = key % 126;
    int bucketindex = -1;
    void *block;
    int index = 1;
    int i = 0;
    int next = -1;
    //Finding the header with the position of the bucket that correspondes to the key
    do
    {
        if (BF_ReadBlock(header_info.fileDesc, index, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        HeaderInfo *h_info;
        h_info = block;
        next = h_info->nextHeaderBlock;
        if (i == headerblock)
        {
            //When I find it I store the buckets position
            memcpy(&bucketindex, block + sizeof(HeaderInfo) + headerindex * sizeof(int), sizeof(int));
            break;
        }
        if (BF_WriteBlock(header_info.fileDesc, index) < 0)
        {
            BF_PrintError("Error writting block");
            return -1;
        }
        index = next;
        i++;
    } while (next != -1);
    next = bucketindex;
    //Searching in every block of the bucket for the record
    do
    {
        bucketindex = next;
        if (BF_ReadBlock(header_info.fileDesc, bucketindex, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        BucketInfo *b_info = block;
        for (int i = 0; i < b_info->numOfRec; i++)
        {
            Record *record = block + sizeof(BucketInfo) + (i * sizeof(Record));
            //If I find it I return the position of the block that it is in
            if (record->id == *((int *)value))
            {
                if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
                {
                    BF_PrintError("Error writting block");
                    return -1;
                }

                return bucketindex;
            }
        }
        //Else I continue searching
        next = b_info->nextBlock;
        if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
        {
            BF_PrintError("Error writting block");
            return -1;
        }
        //I search in every block of the bucket
    } while (next != -1);
    //If I dont find it I return -1
    return -1;
}

int HashStatistics(char *fileName)
{
    HT_info *returnedInfo = HT_OpenIndex(fileName);
    if (returnedInfo == NULL)
    {
        BF_PrintError("Error Opening Index\n");
        return -1;
    }
    HT_info info;
    info.fileDesc = returnedInfo->fileDesc;
    info.attrType = returnedInfo->attrType;
    info.attrName = returnedInfo->attrName;
    info.attrLength = returnedInfo->attrLength;
    info.numBuckets = returnedInfo->numBuckets;
    info.firstHeaderBlock = returnedInfo->firstHeaderBlock;
    int minNumOfRecords = 1000000;
    int maxNumOfRecords = 0;
    long int sumOfRecords = 0;
    int sumOfblocks = 0;
    int overflowBlocks[info.numBuckets];
    int bucketsWithOverflow = 0;
    void *block;

    for (int i = 0; i < info.numBuckets; i++)
    {
        overflowBlocks[i] = 0;
        // 126 is the max number of bucket positions a header block can store
        //Calculating the header block where the position of bucket with that key is located
        int headerblock = i / 126;
        //Calculating the position in the header block where the bucket position is located
        int headerindex = i % 126;
        int bucketindex = -1;
        void *block;
        int sumOfBlockRec = 0;
        int index = info.firstHeaderBlock;
        int k = 0;
        int next = -1;
        //Finding the position of the bucket from the header blocks
        do
        {
            if (BF_ReadBlock(info.fileDesc, index, &block) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            HeaderInfo *h_info;
            h_info = block;
            next = h_info->nextHeaderBlock;
            if (k == headerblock)
            {
                //When I find it I store the buckets position
                memcpy(&bucketindex, block + sizeof(HeaderInfo) + headerindex * sizeof(int), sizeof(int));
                break;
            }
            if (BF_WriteBlock(info.fileDesc, index) < 0)
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            index = next;
            k++;
        } while (next != -1);
        next = bucketindex;
        //Traversing all blocks in the bucket and extracting the information
        do
        {
            bucketindex = next;
            if (BF_ReadBlock(info.fileDesc, bucketindex, &block) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            BucketInfo *b_info = block;
            sumOfBlockRec += b_info->numOfRec;
            sumOfRecords += b_info->numOfRec;
            sumOfblocks++;
            next = b_info->nextBlock;
            overflowBlocks[i]++;
            if (BF_WriteBlock(info.fileDesc, bucketindex) < 0)
            {
                BF_PrintError("Error writting block");
                return -1;
            }
        } while (next != -1);
        if (sumOfBlockRec > maxNumOfRecords)
        {
            maxNumOfRecords = sumOfBlockRec;
        }
        if (sumOfBlockRec < minNumOfRecords)
        {
            minNumOfRecords = sumOfBlockRec;
        }
        //If a bucket has more than 1 blocks this means that it has (overflowBlocks[i]-1)blocks that overflowed
        if (overflowBlocks[i] > 1)
        {
            bucketsWithOverflow++;
        }
    }
    printf("\n-------------HT HASH STATISTICS-------------\n");
    //-----------------A-----------------//
    //Printing the amount of blocks in the file
    printf("\nThe number of Blocks in the file are: %d\n", BF_GetBlockCounter(info.fileDesc));

    //-----------------B-----------------//
    //Printing the max, min and average Records in the buckets
    printf("The max count of records in the buckets are: %d\n", maxNumOfRecords);
    printf("The min count of records in the buckets are: %d\n", minNumOfRecords);
    printf("The average count of records in the buckets are: %f\n", (float)sumOfRecords / (float)info.numBuckets);

    //-----------------C-----------------//
    //Printing the average amoount of bloocks in the buckets
    printf("The average count of blocks in the buckets are: %f\n", (float)sumOfblocks / (float)info.numBuckets);

    //-----------------D-----------------//
    //Printing the amount of buckets that have overflow
    printf("The count of buckets that have overflow blocks are: %d\n", bucketsWithOverflow);
    for (int i = 0; i < info.numBuckets; i++)
    {
        //Printing the buckets that overflowed along side the number of blocks that overflowed in each bucket
        if (overflowBlocks[i] > 1)
        {
            printf("The bucket %d has %d blocks that overflowed\n", i, overflowBlocks[i] - 1);
        }
    }
    if (HT_CloseIndex(returnedInfo) < 0)
    {
        perror("Error closing file\n");
        return -1;
    }
    return 0;
}
