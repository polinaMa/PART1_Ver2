//
// Created by Polina on 12-Mar-18.
//

#include "Directory.h"


/* ****************** START ****************** Directory STRUCT Definition ****************** START ***************** */

/*
 * copy_directory_info - returns pointer to a copy of the serial number of the directory received as input
 *
 * @directory_info - pointer to the serial number of the directory to be copied
 */
static ListElement copy_directory_info(ListElement directory_info , PMemory_pool mem_pool){
    assert(directory_info);
    unsigned long* sn = (unsigned long*)(directory_info);
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
static  void free_dir_info(ListElement directory_info){
    //All is allocated in POOL - Nothing to Free
    //Function exists for Compatibility to List Structure
    return;
}


/* ******************* END ****************** Directory STRUCT Definition ****************** END ******************** */
/* ****************************************************************************************************************** */
/* ****************************************************************************************************************** */
/* ****************** START ****************** Directory STRUCT Functions ****************** START ****************** */
Dir dir_create(char* dir_id , unsigned int depth , unsigned long dir_sn , char* parent_dir_id ,PMemory_pool mem_pool){
    Dir dir = memory_pool_alloc(mem_pool , sizeof(*dir));
    if(dir == NULL){
        return NULL;
    }

    dir->object_id = memory_pool_alloc(mem_pool , (sizeof(char)*(strlen(dir_id) + 1)));
    if(!(dir->object_id)){
        //All is allocated in POOL - Nothing to Free
        return NULL;
    }
    dir->object_id = strcpy(dir->object_id , dir_id);

    dir->parent_dir_id = memory_pool_alloc(mem_pool , (sizeof(char)*(strlen(parent_dir_id) + 1)));
    if(!(dir->parent_dir_id)){
        //All is allocated in POOL - Nothing to Free
        return NULL;
    }
    dir->parent_dir_id = strcpy(dir->parent_dir_id , parent_dir_id);

    dir->dir_depth = depth;
    dir->object_sn = dir_sn;
    dir->num_of_files = 0;
    dir->num_of_subdirs = 0;
    dir->parent_dir_sn = 0; //  not known in the time of creation
    dir->dirs_list = listCreate_pool(copy_directory_info , free_dir_info , mem_pool);
    dir->files_list = listCreate_pool(copy_directory_info , free_dir_info , mem_pool);

    if((!dir->files_list) || (!dir->dirs_list)){
        //All is allocated in POOL - Nothing to Free
        return NULL;
    }
    return dir;
}

ErrorCode dir_set_parent_dir_sn(Dir dir , unsigned long sn){
    assert(dir);
    dir->parent_dir_sn = sn;
    return SUCCESS;
}

ErrorCode dir_add_file(Dir dir , unsigned long file_sn , PMemory_pool mem_pool){
    if(dir == NULL || file_sn < 0){
        return INVALID_INPUT;
    }
    unsigned long* temp = malloc(sizeof(*temp));
    if(temp == NULL){
        return OUT_OF_MEMORY;
    }

    *temp = file_sn;
    ListResult res = listInsertFirst_pool(dir->files_list , temp ,mem_pool);
    if(res != LIST_SUCCESS){
        free(temp);
        return OUT_OF_MEMORY;
    }

    (dir->num_of_files)++;
    free(temp);
    return SUCCESS;
}

ErrorCode dir_add_sub_dir(Dir dir , unsigned long dir_sn , PMemory_pool mem_pool){
    if(dir == NULL || dir_sn < 0){
        return INVALID_INPUT;
    }
    unsigned long* temp = malloc(sizeof(*temp));
    if(temp == NULL){
        return OUT_OF_MEMORY;
    }
    *temp = dir_sn;
    ListResult res = listInsertFirst_pool(dir->dirs_list, temp , mem_pool);
    if(res != LIST_SUCCESS){
        free(temp);
        return OUT_OF_MEMORY;
    }
    (dir->num_of_subdirs)++;
    free(temp);
    return SUCCESS;
}

/* ******************* END ******************* Directory STRUCT Functions ******************* END ******************* */
