#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BF.h"
#include "SHT.h"
#include "HT.h"

#define FILENAMESHT "build/secondaryHashFile"
#define FILENAMEHT "build/hashFile"
#define MAX_FILES 100
#define MAX_BLOCKS 500

int main(int argc, char **argv)
{
    //Getting information about the method, the input file and the buckets(only SHT)
    //I'm going to use from the command line
    if (argc != 4)
    {
        printf("Wrong amount of arguments: \nThe correct format is:\n./test (integer with value 1 or 5 or 10 or 15) (method to be used SHT) (The amount of buckets to be used)\n");
        return -1;
    }
    char numOfRecs[10];
    char method[10];
    char numOfBuckets[10];
    strcpy(numOfRecs, argv[1]);
    strcpy(method, argv[2]);

    if (!strcmp(method, "SHT") && (atoi(numOfRecs) == 1 || atoi(numOfRecs) == 5 || atoi(numOfRecs) == 10 || atoi(numOfRecs) == 15))
    {
        printf("SHT here!!\n");
        if (argc != 4)
        {
            printf("Wrong amount of arguments: \nThe correct format is:\n./bin/main (integer with value 1 or 5 or 10 or 15) SHT (The amount of buckets to be used)\n");
            return -1;
        }
        strcpy(numOfBuckets, argv[3]);
        for (int i = 0, n = strlen(numOfBuckets); i < n; i++)
        {
            if (numOfBuckets[i] < 48 || numOfBuckets[i] > 57)
            {
                printf("Error! The number of buckets can only be an integer!\n");
                printf("The correct format is:\n./bin/main (integer with value 1 or 5 or 10 or 15) SHT (The amount of buckets to be used)\n");
                return -1;
            }
        }
    }
    else
    {
        printf("Wrong format of arguments: \nThe correct format is:\n./bin/main (integer with value 1 or 5 or 10 or 15) SHT (The amount of buckets to be used)\n");
        return -1;
    }
    //Extraacting records from the file according to the argv input
    char file[25];
    strcpy(file, "examples/records");
    strcat(file, numOfRecs);
    strcat(file, "K.txt");
    FILE *fileInput = fopen(file, "r");
    int entryCount = atoi(numOfRecs) * 1000;
    Record recs[entryCount];
    int i, j;
    char *koma = ",";
    char *key;
    char input[110];
    char strings[4][50];
    int id;

    for (i = 0; i < entryCount; i++)
    {
        fscanf(fileInput, "%s", input);

        key = strtok(input, koma);
        j = 0;
        for (j = 0; key != NULL; j++)
        {
            strcpy(strings[j], key);
            key = strtok(NULL, koma);
        }

        sscanf(strings[0], "{%d", &id);

        int k;
        for (j = 1; j < 4; j++)
        {
            for (k = 0; k < strlen(strings[j]) - 1; k++)
            {
                strings[j][k] = strings[j][k + 1];
            }
            strings[j][k - 1] = '\0';
        }
        strings[3][k - 2] = '\0';
        //Copying  the records
        recs[i].id = id;
        strcpy(recs[i].name, strings[1]);
        strcpy(recs[i].surname, strings[2]);
        strcpy(recs[i].address, strings[3]);
    }
    //Initializing viriables
    BF_Init();
    int insertedSucc = 0;
    int insertedUnsucc = 0;
    int SHTinsertedSucc = 0;
    int SHTinsertedUnsucc = 0;
    int foundBeforeDelete = 0;
    int notFoundBeforeDeleted = 0;
    int SHTfoundBeforeDelete = 0;
    int SHTnotFoundBeforeDeleted = 0;
    int foundAfterDelete = 0;
    int notFoundAfterDelete = 0;
    int deletedSucc = 0;
    int deletedUnsucc = 0;
    int totalBlocksRead = 0;
    if (!strcmp(method, "SHT"))
    {
        //Creating file
        if (HT_CreateIndex(FILENAMEHT, 'c', "keyht", 5, atoi(numOfBuckets)) < 0)
        {
            perror("Error creating file\n");
            exit(1);
        }

        HT_info *ht_info = HT_OpenIndex(FILENAMEHT);
        if (ht_info == NULL)
        {
            printf("Error opening file");
        }

        if (SHT_CreateSecondaryIndex(FILENAMESHT, "keysht", 6, atoi(numOfBuckets), FILENAMEHT) < 0)
        {
            perror("Error creating file\n");
            exit(1);
        }
        SHT_info *sht_info = SHT_OpenSecondaryIndex(FILENAMESHT);
        if (sht_info == NULL)
        {
            printf("Error opening file");
        }
        //Inserting all records from input file to the file I created
        for (i = 0; i < entryCount; i++)
        {
            int tt;
            if ((tt = HT_InsertEntry(*ht_info, recs[i])) > 0)
            {
                printf("Entry inserted correctly in HT\n");
                insertedSucc++;
                SecondaryRecord rec;
                rec.blockId = tt;
                strcpy(rec.surname, recs[i].surname);
                //Iserting secondary record to the secondary hash file if they were correctly inserted in the primary hash file
                if (SHT_SecondaryInsertEntry(*sht_info, rec) > 0)
                {
                    printf("Entry inserted correctly in SHT\n");
                    SHTinsertedSucc++;
                }
                else
                {
                    printf("couldn't insert entry in SHT\n");
                    SHTinsertedUnsucc++;
                }
            }
            else
            {
                printf("couldn't insert entry in HT\n");
                insertedUnsucc++;
            }
        }
        //Checking if they are there
        for (i = 0; i < entryCount; i++)
        {
            if (HT_GetAllEntries(*ht_info, &recs[i].id) > 0)
            {
                printf("Entry found in HT\n");
                foundBeforeDelete++;
            }
            else
            {
                printf("Entry not found in HT\n");
                notFoundBeforeDeleted++;
            }
        }

        for (i = 0; i < entryCount; i++)
        {
            int blocksRead;
            if ((blocksRead = SHT_SecondaryGetAllEntries(*sht_info, *ht_info, &recs[i].surname)) > 0)
            {
                printf("Read %d blocks to find entry\n", blocksRead);
                printf("Entry found in SHT\n");
                totalBlocksRead += blocksRead;
                SHTfoundBeforeDelete++;
            }
            else
            {
                printf("Entry not found in SHT\n");
                SHTnotFoundBeforeDeleted++;
            }
        }
        SHT_CloseSecondaryIndex(sht_info);
        //Closing files
        HT_CloseIndex(ht_info);
        //Printing hash statistics for both ht files
        if (HashStatistics(FILENAMEHT) < 0)
        {
            printf("Error getting statistics\n");
        }

        if (SHT_HashStatistics(FILENAMESHT) < 0)
        {
            printf("Error getting statistics\n");
        }
    }
    //Else Error
    else
    {
        perror("Error");
        return -1;
    }
    //Printing usefull info
    printf("\nMethod used: %s\n", method);
    if (!strcmp(method, "SHT"))
    {
        printf("Number of buckets used: %s\n", numOfBuckets);
    }
    printf("Input file used: %s\n\n", file);
    printf("Inserted in HT %d entries successfully\n", insertedSucc);
    printf("Inserted in HT %d entries unsuccessfully\n", insertedUnsucc);
    printf("Inserted in SHT %d entries successfully\n", SHTinsertedSucc);
    printf("Inserted in SHT %d entries unsuccessfully\n", SHTinsertedUnsucc);
    printf("Found %d entries in HT\n", foundBeforeDelete);
    printf("Did not find %d entries in HT\n", notFoundBeforeDeleted);
    printf("Found %d entries in SHT\n", SHTfoundBeforeDelete);
    printf("Did not find %d entries in SHT\n", SHTnotFoundBeforeDeleted);
    printf("Total blocks before finding all secondary entries: %d\n", totalBlocksRead);
    return 0;
}
