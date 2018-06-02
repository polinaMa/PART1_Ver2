//
// Created by Polina on 11-Dec-17.
//

#ifndef DEDUPLICATION_PROJECT_BLOCK_H
#define DEDUPLICATION_PROJECT_BLOCK_H

#include "HashTableF.h"
#include "Utilities.h"

/* ******************* START ****************** Block STRUCT Definition ****************** START ******************** */
/*
 * Definition of a block structure:
 *                  - block_sn -> a running index on all blocks read from the file system
 *                  - block_id -> a hushed id as appears in the input file
 *                  - block_size -> the size of a block
 *                  - shared_by_num_files -> number of files sharing this block
 *                  - files_list -> list of hashed file ids containing this block
 */
struct block_t{
    unsigned long block_sn; // running index
    char* block_id; // Hashed
    unsigned int block_size;
    unsigned int shared_by_num_files;
    HashTableF files_ht;
};
typedef struct block_t *Block;

/* ******************** END ******************** Block STRUCT Definition ******************** END ******************* */
/* ****************************************************************************************************************** */
/* ****************************************************************************************************************** */
/* ******************* START ******************* Block STRUCT Functions ******************* START ******************* */
/*
 *  blockCreate - Creates a new Block with:
 *                      - a given serial number
 *                      - a hashed id
 *                      - creates an empty files list
 *                      - zeros the counter that contains the amount of files sharing this block
 *
 * @block_id   - the hashed id of the block
 * @block_sn   - serial number of the block
 * @block_size - the size of the block
 */
Block block_create(char* block_id , unsigned long block_sn ,
                   unsigned int block_size,PMemory_pool mem_pool);

/*
 *  block_add_file - adds the file containing the block to the files list saved in the block
 *
 *  @block   - pointer to the block structure to which we want to add the file
 *  @file_id - the id of the file that contains the block
 */
ErrorCode block_add_file(Block block , char* file_id , PMemory_pool mem_pool);


/* ******************** END ******************** Block STRUCT Functions ******************** END ******************** */

#endif //DEDUPLICATION_PROJECT_BLOCK_H
