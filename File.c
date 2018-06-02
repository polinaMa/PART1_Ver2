//
// Created by Polina on 12-Mar-18.
//
#include "File.h"
/* ******************* START ******************* File STRUCT Functions ******************* START ******************** */
File file_create(char* file_id , unsigned int depth , unsigned long file_sn , unsigned int size ,
                 unsigned long physical_sn , char dedup_type , PMemory_pool mem_pool){

    File file = memory_pool_alloc(mem_pool , sizeof(*file));
    if(file == NULL){
        return NULL;
    }

    file->object_id = memory_pool_alloc(mem_pool , sizeof(char)* (FILE_ID_LEN + 1));
    if(file->object_id == NULL){
        return NULL; //All is allocated in POOL - Nothing to Free
    }

    file->object_id = strcpy(file->object_id , file_id);
    file->object_sn = file_sn;
    file->parent_dir_sn = 0; //not known in the time of creation
    file->object_type = 'F';

    file->num_blocks = 0;
    file->file_depth = depth;
    file->file_size = size;
    file->num_files = 1;
    file->flag = 'P';
    file->physical_sn = physical_sn; // will be updated from file_compare

    file->blocks_list = listCreate_pool(copy_block_info , free_block_info , mem_pool);
    if(file->blocks_list == NULL){
        return NULL; //All is allocated in POOL - Nothing to Free
    }

    if(dedup_type == 'F'){
        file->logical_files_list = listCreate_pool(copy_sn , free_sn , mem_pool);
        if(file->blocks_list == NULL){
            return NULL; //All is allocated in POOL - Nothing to Free
        }
        listInsertLast_pool(file->logical_files_list , &(file->object_sn) , mem_pool);
    }

    return file;
}

ErrorCode file_set_parent_dir_sn(File file , unsigned long parent_dir_sn){
    assert(file);
    file->parent_dir_sn = parent_dir_sn;
    return SUCCESS;
}

ErrorCode file_set_physical_sn(File file , unsigned long physical_file_sn){
    assert(file);
    file->physical_sn = physical_file_sn;
    return SUCCESS;
}

ErrorCode file_set_logical_flag(File file){
    file->flag = 'L';
    return SUCCESS;
}

ErrorCode file_add_block(File file , char* block_id , int block_size , PMemory_pool mem_pool){
    if(file == NULL || block_id == NULL || block_size < 0){
        return INVALID_INPUT;
    }

    Block_Info bi = malloc(sizeof(*bi)); //Temporary Block Info - Do not allocate in POOL !!!!!
    if(bi == NULL){
        return OUT_OF_MEMORY;
    }
    bi->id =  malloc(sizeof(char)*(strlen(block_id) +1)); //Temporary Block Info - Do not allocate in POOL !!!!!
    if(bi->id == NULL){
        free(bi);
        return OUT_OF_MEMORY;
    }
    strcpy(bi->id , block_id);
    bi->size = block_size;

    ListResult res = listInsertLast_pool(file->blocks_list , bi , mem_pool); // This will be allocated on the memory pool

    if(res != LIST_SUCCESS){
        free(bi->id);
        free(bi);
        return OUT_OF_MEMORY;
    }

    (file->num_blocks)++;
    free(bi->id);
    free(bi);
    return SUCCESS;
}

/* ******************** END ******************** File STRUCT Functions ******************** END ********************* */

