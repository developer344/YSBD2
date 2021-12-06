#ifndef HT_H_
#define HT_H_
#include "BF.h"

typedef struct
{
    int fileDesc;         /* αναγνωριστικός αριθμός ανοίγματος αρχείου από το επίπεδο block */
    char attrType;        /* ο τύπος του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο, 'c' ή'i' */
    char *attrName;       /* το όνομα του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο */
    int attrLength;       /* το μέγεθος του πεδίου που είναι κλειδί για το συγκεκριμένο αρχείο */
    long int numBuckets;  /* το πλήθος των “κάδων” του αρχείου κατακερματισμού */
    int firstHeaderBlock; /* The first header block after the information block*/
} HT_info;

//Info fr the header blocks
typedef struct
{
    int nextHeaderBlock; //Next header block that contains the positions of the bucket
    int bytesLeft;       //The bytes leftin the block
} HeaderInfo;

//Info for the buckets
typedef struct
{
    int nextBlock; //The position of the next block in the bucket
    int key;       //The key of the buckeet
    int bytesLeft; //The bytes left in the block of the bucket
    int numOfRec;  //The number of records stored in the block of the bucket
} BucketInfo;

int HT_CreateIndex(char *fileName, char attrType, char *attrName, int attrLength, int buckets);

HT_info *HT_OpenIndex(char *fileName);

int HT_CloseIndex(HT_info *header_info);

int HT_InsertEntry(HT_info header_info, Record record);

int HT_DeleteEntry(HT_info header_info, void *value);

int HT_GetAllEntries(HT_info header_info, void *value);

int HashStatistics(char *fileName);
#endif /* HT_H_ */