//
// Created by Polina on 12-Mar-18.
//

#include "Block.h"

/* ******************* START ******************* Block STRUCT Functions ******************* START ******************* */

Block block_create(char* block_id , unsigned long block_sn ,
                   unsigned int block_size, PMemory_pool mem_pool){
    Block block = memory_pool_alloc(mem_pool , sizeof(*block)); //create a block
    if(block == NULL){ //Check memory allocation was successful
        return NULL;
    }

    block->block_id = memory_pool_alloc(mem_pool , sizeof(char)*(BLOCK_ID_LEN + 1)); //allocate string for block_id
    if(block->block_id == NULL){ //check successful allocation
        //All is allocated in POOL - Nothing to Free
        return NULL;
    }
    block->block_id = strcpy(block->block_id , block_id);
    block->block_sn = block_sn;
    block->shared_by_num_files = 0;
    block->block_size = block_size;

    block->files_ht = ht_createF('N' , mem_pool);
    if(block->files_ht == NULL){
        //All is allocated in POOL - Nothing to Free
        return NULL;
    }
    return block;
}

ErrorCode block_add_file(Block block , char* file_id, PMemory_pool mem_pool){
    if(file_id == NULL || block == NULL){ //Check input is valid
        return INVALID_INPUT;
    }
    bool object_exists = false;

    EntryF result = ht_setF(block->files_ht, file_id, &object_exists ,mem_pool);
    if(result == NULL){ //Check for memory allocation
        return OUT_OF_MEMORY;
    }

    if(object_exists == false){
        (block->shared_by_num_files)++;
    }

    return SUCCESS;
}

/* ******************** END ******************** Block STRUCT Functions ******************** END ******************** */


