#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "SHT.h"
#include "BF.h"

int SHT_CreateSecondaryIndex(char *sfileName, char *attrName, int attrLength, int buckets, char *fileName)
{
    SHT_info *sht_info = malloc(sizeof(SHT_info));
    if (BF_CreateFile(sfileName) < 0)
    {
        BF_PrintError("Error creating file");
        return -1;
    }
    if ((sht_info->fileDesc = BF_OpenFile(sfileName)) < 0)
    {
        BF_PrintError("Error opening file\n");
        return -1;
    }
    if (BF_AllocateBlock(sht_info->fileDesc) < 0)
    {
        BF_PrintError("Error allocating block");
        return -1;
    }
    void *blockh;
    if (BF_ReadBlock(sht_info->fileDesc, 0, &blockh) < 0)
    {
        BF_PrintError("Error reading block");
        return -1;
    }
    //Initialisation of SHT_info
    sht_info->attrLength = attrLength;
    sht_info->attrName = malloc(sizeof(char) * sht_info->attrLength);
    strncpy(sht_info->attrName, attrName, attrLength);
    sht_info->numBuckets = buckets;
    sht_info->firstHeaderBlock = 1;
    sht_info->fileName = malloc(sizeof(char) * 20);
    strcpy(sht_info->fileName, fileName);
    //First block is the first header sht_info block and only has HP sht_info
    memcpy(blockh, sht_info, sizeof(SHT_info));
    if (BF_WriteBlock(sht_info->fileDesc, 0))
    {
        BF_PrintError("Error writting block");
        return -1;
    }
    if (BF_AllocateBlock(sht_info->fileDesc) < 0)
    {
        BF_PrintError("Error allocating block");
        return -1;
    }
    void *block;
    if (BF_ReadBlock(sht_info->fileDesc, BF_GetBlockCounter(sht_info->fileDesc) - 1, &block) < 0)
    {
        BF_PrintError("Error reading block");
        return -1;
    }
    //Creating frst header block
    //First header block and the other header blocks have the number of the block
    //that corresponds to the buckets.

    //Every header block has the bytes left in the block,
    //the pisition of the next header block and the pisitions of at most 126 buckets.
    SHeaderInfo *h_info = malloc(sizeof(SHeaderInfo));
    h_info->bytesLeft = BLOCK_SIZE - sizeof(SHeaderInfo);
    h_info->nextHeaderBlock = -1;
    memcpy(block, h_info, sizeof(SHeaderInfo));
    if (BF_WriteBlock(sht_info->fileDesc, BF_GetBlockCounter(sht_info->fileDesc) - 1) < 0)
    {
        BF_PrintError("Error writting block");
        return -1;
    }
    int HB_offset = 0;
    int headerblockindex = BF_GetBlockCounter(sht_info->fileDesc) - 1; // block
    int bucketposition;
    for (int i = 0; i < buckets; i++)
    {
        //Allocating bucket blocks
        if (BF_AllocateBlock(sht_info->fileDesc) < 0)
        {
            BF_PrintError("Error allocating block");
            return -1;
        }
        if (BF_ReadBlock(sht_info->fileDesc, (bucketposition = BF_GetBlockCounter(sht_info->fileDesc) - 1), &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        //Initializing bucket blocks
        SBucketInfo *b_info = malloc(sizeof(SBucketInfo));
        b_info->numOfRec = 0;
        b_info->nextBlock = -1;
        b_info->key = i;
        b_info->bytesLeft = BLOCK_SIZE - sizeof(SBucketInfo);
        memcpy(block, b_info, sizeof(SBucketInfo));
        if (BF_WriteBlock(sht_info->fileDesc, bucketposition) < 0)
        {
            BF_PrintError("Error writting block");
            return -1;
        }
        if (BF_ReadBlock(sht_info->fileDesc, headerblockindex, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        //If the header block is full I create a new one to contue referencing the header blocks
        SHeaderInfo *head_info = block;
        if (head_info->bytesLeft < sizeof(int)) //bytesleft
        {
            if (BF_AllocateBlock(sht_info->fileDesc) < 0)
            {
                BF_PrintError("Error allocating block");
                return -1;
            }
            //Allocating new header block putting its position to the variable (nextHeaderBlock)
            //of the previous header block
            int nextheaderblock = BF_GetBlockCounter(sht_info->fileDesc) - 1;
            head_info->nextHeaderBlock = nextheaderblock;
            memcpy(block, head_info, sizeof(SHeaderInfo));
            if (BF_WriteBlock(sht_info->fileDesc, headerblockindex))
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            headerblockindex = nextheaderblock;
            if (BF_ReadBlock(sht_info->fileDesc, headerblockindex, &block) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            //Initializing new header block
            h_info = malloc(sizeof(SHeaderInfo));
            h_info->bytesLeft = BLOCK_SIZE - sizeof(SHeaderInfo);
            h_info->nextHeaderBlock = -1;
            memcpy(block, h_info, sizeof(SHeaderInfo));
            free(h_info);
            HB_offset = 0;
        }
        //Puttting the position of the last allocated bucket t the currrent headerblock so I can reference it later
        head_info = block;
        memcpy(block + sizeof(SHeaderInfo) + HB_offset * sizeof(int), &bucketposition, sizeof(int));
        head_info->bytesLeft -= sizeof(int);
        memcpy(block, head_info, sizeof(SHeaderInfo));
        HB_offset++;
        if (BF_WriteBlock(sht_info->fileDesc, headerblockindex))
        {
            BF_PrintError("Error writting block");
            return -1;
        }
    }
    //Closing index
    if (BF_CloseFile(sht_info->fileDesc) < 0)
    {
        BF_PrintError("Error closing file");
        return -1;
    }
    return 0;
}

SHT_info *SHT_OpenSecondaryIndex(char *sfileName)
{
    int fDesc;
    //Opening file and returning header sht_info
    if ((fDesc = BF_OpenFile(sfileName)) < 0)
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
    SHT_info *returnValue = malloc(sizeof(SHT_info));
    memcpy(returnValue, block, sizeof(SHT_info));
    if (BF_WriteBlock(fDesc, 0))
    {
        BF_PrintError("Error writting block");
        return NULL;
    }
    return returnValue;
}

int SHT_CloseSecondaryIndex(SHT_info *header_info)
{
    //Closing file

    if (BF_CloseFile(header_info->fileDesc) < 0)
    {
        BF_PrintError("Error closing file");
        return -1;
    }
    return 0;
}

int SHT_SecondaryInsertEntry(SHT_info header_info, SecondaryRecord record)
{
    int key = 0;
    for (int i = 0; i < strlen(record.surname); i++)
    {

        key += record.surname[i];
    }
    key = key % header_info.numBuckets;
    int headerblock = key / 126;
    //Calculating the position in the header block where the bucket position is located
    int headerindex = key % 126;
    int bucketindex = -1;
    void *block;
    int index = 1;
    int i = 0;
    int next = -1;
    //Finding the header with the position of the bucket that correspondes to the key
    SHeaderInfo *h_info;
    do
    {
        if (BF_ReadBlock(header_info.fileDesc, index, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        h_info = block;
        next = h_info->nextHeaderBlock;
        if (i == headerblock)
        {
            //When I find it I store the buckets position
            memcpy(&bucketindex, block + sizeof(SHeaderInfo) + headerindex * sizeof(int), sizeof(int));
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
    SBucketInfo *b_info;
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
        //Each block can be at most 80% full
        if (b_info->bytesLeft > sizeof(SecondaryRecord) + BLOCK_SIZE * 0.2)
        {
            //If I find a block with space I store te record to the last position of the block

            //Storing record
            memcpy(blockb + sizeof(SBucketInfo) + b_info->numOfRec * (int)sizeof(SecondaryRecord), &record, sizeof(SecondaryRecord));
            b_info->numOfRec += 1;
            b_info->bytesLeft -= sizeof(SecondaryRecord);
            memcpy(blockb, b_info, sizeof(SBucketInfo));
            //Storing the new information for the block
            if (BF_WriteBlock(header_info.fileDesc, bucketindex) < 0)
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            printf("\nRecord with surname: %s\n", record.surname);
            printf("In the secondary Hash table: Added to Block: %d\n\n", bucketindex);
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
            SBucketInfo *b_info_new = malloc(sizeof(SBucketInfo));
            b_info_new->key = key;
            b_info_new->bytesLeft = BLOCK_SIZE - sizeof(SBucketInfo) - sizeof(SecondaryRecord);
            b_info_new->numOfRec = 1;
            b_info_new->nextBlock = -1;
            memcpy(blockbucket, b_info_new, sizeof(SBucketInfo));
            //Storing the record to the first position of the block
            memcpy(blockbucket + sizeof(SBucketInfo), &record, sizeof(SecondaryRecord));

            if (BF_WriteBlock(header_info.fileDesc, index) < 0)
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            //Storing the position of the newly allocated block to the previous block
            b_info->nextBlock = index;
            memcpy(blockb, b_info, sizeof(SBucketInfo));
            if (BF_WriteBlock(header_info.fileDesc, bucketindex))
            {
                BF_PrintError("Error writting block");
                return -1;
            }
            printf("\nRecord with surname: %s\n", record.surname);
            printf("In the secondary Hash table: Added to Block: %d\n\n", bucketindex);
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

int SHT_SecondaryGetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value)
{
    //Calculating the key
    int key = 0;
    for (int i = 0; i < strlen((char *)value); i++)
    {
        key += ((char *)value)[i];
    }
    key = key % header_info_sht.numBuckets;
    // 126 is the max number of bucket positions a header block can store
    //Calculating the header block where the position of bucket with that key is located
    int headerblock = key / 126;
    //Calculating the position in the header block where the bucket position is located
    int headerindex = key % 126;
    int bucketindex = -1;
    void *block;
    void *block_ht;
    int index = 1;
    int i = 0;
    int next = -1;
    int blocksRead = 0;
    //Finding the header with the position of the bucket that correspondes to the key
    SHeaderInfo *h_info;
    do
    {
        if (BF_ReadBlock(header_info_sht.fileDesc, index, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }

        h_info = block;
        next = h_info->nextHeaderBlock;
        if (i == headerblock)
        {
            //When I find it I store the buckets position
            memcpy(&bucketindex, block + sizeof(SHeaderInfo) + headerindex * sizeof(int), sizeof(int));
            break;
        }
        if (BF_WriteBlock(header_info_sht.fileDesc, index) < 0)
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
        if (BF_ReadBlock(header_info_sht.fileDesc, bucketindex, &block) < 0)
        {
            BF_PrintError("Error reading block");
            return -1;
        }
        SBucketInfo *b_info = block;
        blocksRead++;
        for (int i = 0; i < b_info->numOfRec; i++)
        {
            SecondaryRecord *record_sht = block + sizeof(SBucketInfo) + (i * sizeof(SecondaryRecord));
            //If I find it I take the block id and I open the corresponding.
            if (!strcmp((char *)value, record_sht->surname))
            {
                int blockid = record_sht->blockId;
                if (BF_ReadBlock(header_info_ht.fileDesc, blockid, &block_ht) < 0)
                {
                    BF_PrintError("Error reading block");
                    return -1;
                }
                BucketInfo *bucketinfo_ht = block_ht;
                bool found = false;
                //I open the block with the block id I extracted from the secondary record.
                for (int j = 0; j < bucketinfo_ht->numOfRec; j++)
                {
                    Record *record_ht = block_ht + sizeof(BucketInfo) + (j * sizeof(Record));
                    //If I find the record in the primary hash file that has the same surname with the secondary record surname in print it .
                    if (!strcmp(record_ht->surname, record_sht->surname))
                    {
                        found = true;
                        printf("Record found!! Id = %d, Name = %s, Surname = %s, Address = %s\n", record_ht->id, record_ht->name, record_ht->surname, record_ht->address);
                    }
                }

                if (BF_WriteBlock(header_info_ht.fileDesc, blockid) < 0)
                {
                    BF_PrintError("Error writting block");
                    return -1;
                }
                if (found == true)
                {
                    return blocksRead;
                }
            }
        }
        //Else I continue searching
        next = b_info->nextBlock;
        if (BF_WriteBlock(header_info_sht.fileDesc, bucketindex) < 0)
        {
            BF_PrintError("Error writting block");
            return -1;
        }
        //I search in every block of the bucket
    } while (next != -1);
    //If I dont find it I return -1
    return -1;
}

int SHT_HashStatistics(char *fileName)
{
    SHT_info *returnedInfo;
    if ((returnedInfo = SHT_OpenSecondaryIndex(fileName)) == NULL)
    {
        perror("Error opening secondary ht file\n");
        return -1;
    }
    HT_info *ht_info;
    if ((ht_info = HT_OpenIndex(returnedInfo->fileName)) == NULL)
    {
        perror("Error opening  primary ht file\n");
        return -1;
    }
    SHT_info sht_info;
    sht_info.fileDesc = returnedInfo->fileDesc;
    sht_info.attrLength = returnedInfo->attrLength;
    sht_info.numBuckets = returnedInfo->numBuckets;
    sht_info.firstHeaderBlock = returnedInfo->firstHeaderBlock;
    sht_info.fileName = returnedInfo->fileName;
    int minNumOfRecords = 1000000;
    int maxNumOfRecords = 0;
    long int sumOfRecords = 0;
    int sumOfblocks = 0;
    int overflowBlocks[sht_info.numBuckets];
    int bucketsWithOverflow = 0;
    void *block;
    for (int i = 0; i < sht_info.numBuckets; i++)
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
        int index = sht_info.firstHeaderBlock;
        int k = 0;
        int next = -1;
        //Finding the position of the bucket from the header blocks
        do
        {
            if (BF_ReadBlock(returnedInfo->fileDesc, index, &block) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            SHeaderInfo *h_info;
            h_info = block;
            next = h_info->nextHeaderBlock;
            if (k == headerblock)
            {
                //When I find it I store the buckets position
                memcpy(&bucketindex, block + sizeof(SHeaderInfo) + headerindex * sizeof(int), sizeof(int));
                break;
            }

            if (BF_WriteBlock(sht_info.fileDesc, index) < 0)
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
            if (BF_ReadBlock(sht_info.fileDesc, bucketindex, &block) < 0)
            {
                BF_PrintError("Error reading block");
                return -1;
            }
            SBucketInfo *b_info = block;
            sumOfBlockRec += b_info->numOfRec;
            sumOfRecords += b_info->numOfRec;
            sumOfblocks++;
            next = b_info->nextBlock;
            overflowBlocks[i]++;
            if (BF_WriteBlock(sht_info.fileDesc, bucketindex) < 0)
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
    printf("\n-------------SHT HASH STATISTICS-------------\n");
    //-----------------A-----------------//
    //Printing the amount of blocks in the file
    printf("\nThe number of Blocks in the file are: %d\n", BF_GetBlockCounter(sht_info.fileDesc));

    //-----------------B-----------------//
    //Printing the max, min and average Records in the buckets
    printf("The max count of records in the buckets are: %d\n", maxNumOfRecords);
    printf("The min count of records in the buckets are: %d\n", minNumOfRecords);
    printf("The average count of records in the buckets are: %f\n", (float)sumOfRecords / (float)sht_info.numBuckets);

    //-----------------C-----------------//
    //Printing the average amoount of bloocks in the buckets
    printf("The average count of blocks in the buckets are: %f\n", (float)sumOfblocks / (float)sht_info.numBuckets);

    //-----------------D-----------------//
    //Printing the amount of buckets that have overflow
    printf("The count of buckets that have overflow blocks are: %d\n", bucketsWithOverflow);
    for (int i = 0; i < sht_info.numBuckets; i++)
    {
        //Printing the buckets that overflowed along side the number of blocks that overflowed in each bucket
        if (overflowBlocks[i] > 1)
        {
            printf("The bucket %d has %d blocks that overflowed\n", i, overflowBlocks[i] - 1);
        }
    }
    if (HT_CloseIndex(ht_info) < 0)
    {
        printf("Error closing primary ht file");
    }
    if (SHT_CloseSecondaryIndex(returnedInfo) < 0)
    {
        perror("Error closing secondary ht file\n");
        return -1;
    }
    return 0;
}