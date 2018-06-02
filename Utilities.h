//
// Created by Polina on 14-Dec-17.
//

#ifndef DEDUPLICATIONPROJECT_UTILITIES_H
#define DEDUPLICATIONPROJECT_UTILITIES_H

typedef enum{
    SUCCESS,
    INVALID_INPUT,
    OUT_OF_MEMORY
} ErrorCode;
#include "List.h"
#include "memory_pool.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ******************************************* Definitions& Magic Numbers ******************************************* */
/* Magic Numbers */
#define STR_OF_Z 12
#define DIR_NAME_LEN 11
#define DIR_NAME_HASH 10
#define BLOCK_ID_LEN 13
#define FILE_ID_LEN 25
#define BUFFER_SIZE 1024
#define LINE_SPACE 10
#define CHUNKE_ID_LEN 10
#define CHUNKE_SIZE_LEN 6
#define DIR_SIZE 0
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define ROOT_ID_LEN 10
#define FILE_SYSTEM_ID_LEN 3


/*
 * copy_directory_info - returns pointer to a copy of the serial number of the directory received as input
 *
 * @directory_info - pointer to the serial number of the directory to be copied
 */
static ListElement copy_sn(ListElement element , PMemory_pool mem_pool){
    assert(element);
    unsigned long* sn = (unsigned long*)(element);
    //unsigned long* sn_copy = malloc(sizeof(*sn_copy));
    unsigned long* sn_copy = memory_pool_alloc(mem_pool , sizeof(*sn_copy));
    if(sn_copy == NULL){
        return NULL;
    }
    *sn_copy = *sn;
    return sn_copy;
}

/*
 * free_dir_info - frees the allocated space to the serial number of a directory
 *
 * @directory_info - pointer to the serial number that should be freed
 */
static  void free_sn(ListElement element){
    //free(element);
    return;
}

/* ******************** START ******************** object_info struct ******************** START ******************** */

/*
 * Definition of a object_info structure:
 *
 */
struct object_info{ //helper struct
    char* object_id;
    unsigned long object_sn;
    char* parent_dir_id;
    char object_type; //can be either 'F' or 'D'
};
typedef struct object_info* Object_Info;

/*
 *
 */
Object_Info object_info_create(char* id , unsigned long sn , char* parent_id , char type);

/*
 *
 */
ListElement object_info_copy(ListElement object_info);

/*
 *
 */
void object_info_destroy(ListElement object_info);


/* ********************* END ********************* object_info struct ********************* END ********************* */
/* ******************** START ******************** block_info struct ******************** START ********************* */
/*
 * Definition of a block info structure:
 *                  - block_sn -> a running index on all blocks read from the file system
 *                  - block_id -> a hushed id as appears in the input file
 *                  - block_size -> the size of a block
 *                  - shared_by_num_files -> number of files sharing this block
 *                  - files_list -> list of hashed file ids containing this block
 */
struct block_info{ //helper struct
    int size;
    char* id; // block id
};
typedef struct block_info* Block_Info;

/*
 *
 */
ListElement copy_block_info(ListElement block_info , PMemory_pool mem_pool);

/*
 *
 */
void free_block_info(ListElement block_info);

/* ********************** END ********************* block_info struct ********************* END ********************* */
#endif //DEDUPLICATIONPROJECT_UTILITIES_H
