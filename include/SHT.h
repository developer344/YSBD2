#ifndef SHT_H_
#define SHT_H_
#include "BF.h"
#include "HT.h"

//Secondary block struct
typedef struct
{
    char surname[25];
    int blockId;
} SecondaryRecord;

typedef struct
{
    int fileDesc;         /* αναγνωριστικός αριθµός ανοίγµατος αρχείου από το επίπεδο block */
    char *attrName;       /* το όνοµα του πεδίου που είναι κλειδί για το συγκεκριµένο αρχείο */
    int attrLength;       /* το µέγεθος του πεδίου που είναι κλειδί για το συγκεκριµένο αρχείο */
    long int numBuckets;  /* το πλήθος των “κάδων” του αρχείου κατακερµατισµού */
    char *fileName;       /* όνοµα αρχείου µε το πρωτεύον ευρετήριο στο id */
    int firstHeaderBlock; /* The first header block after the information block*/
} SHT_info;

//Info fr the header blocks
typedef struct
{
    int nextHeaderBlock; //Next header block that contains the positions of the bucket
    int bytesLeft;       //The bytes leftin the block
} SHeaderInfo;

//Info for the buckets
typedef struct
{
    int nextBlock; //The position of the next block in the bucket
    int key;       //The key of the buckeet
    int bytesLeft; //The bytes left in the block of the bucket
    int numOfRec;  //The number of records stored in the block of the bucket
} SBucketInfo;

int SHT_CreateSecondaryIndex(char *sfileName, char *attrName, int attrLength, int buckets, char *fileName);

SHT_info *SHT_OpenSecondaryIndex(char *sfileName);

int SHT_CloseSecondaryIndex(SHT_info *header_info_sht);

int SHT_SecondaryInsertEntry(SHT_info header_info_sht, SecondaryRecord record);

int SHT_SecondaryGetAllEntries(SHT_info header_info_sht, HT_info header_info_ht, void *value);

int SHT_HashStatistics(char *fileName);
#endif /* SHT_H_ */