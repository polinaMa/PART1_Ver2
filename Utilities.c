//
// Created by Polina on 12-Mar-18.
//

#include "Utilities.h"

/* ******************** START ******************** object_info struct ******************** START ******************** */

Object_Info object_info_create(char* id , unsigned long sn , char* parent_id , char type){
    Object_Info oi = malloc(sizeof(*oi));
    if(oi == NULL){
        return NULL;
    }
    //Set numeric Values
    oi->object_sn = sn;
    oi->object_type = type;

    //Set object_id
    oi->object_id = malloc(sizeof(char)*(strlen(id) +1));
    if(oi->object_id == NULL){
        free(oi);
        return NULL;
    }
    strcpy(oi->object_id  , id);

    //Set parent_dir_id
    oi->parent_dir_id = malloc(sizeof(char)*(strlen(parent_id) +1));
    if(oi->object_id == NULL){
        free(oi->parent_dir_id);
        free(oi);
        return NULL;
    }
    strcpy(oi->parent_dir_id  , parent_id);

    return oi;
}

ListElement object_info_copy(ListElement object_info){
    assert(object_info);
    Object_Info oi = (Object_Info)(object_info);
    Object_Info oi_copy = malloc(sizeof(*oi_copy));
    if(oi_copy == NULL){
        return NULL;
    }
    //Copy numeric Values
    oi_copy->object_sn = oi->object_sn;
    oi_copy->object_type = oi->object_type;

    //copy object_id
    oi_copy->object_id = malloc(sizeof(char)*(strlen(oi->object_id) +1));
    if(oi_copy->object_id == NULL){
        free(oi_copy);
        return NULL;
    }
    strcpy(oi_copy->object_id  , oi->object_id);

    //copy parent_dir_id
    oi_copy->parent_dir_id = malloc(sizeof(char)*(strlen(oi->parent_dir_id) +1));
    if(oi_copy->object_id == NULL){
        free(oi_copy->parent_dir_id);
        free(oi_copy);
        return NULL;
    }
    strcpy(oi_copy->parent_dir_id  , oi->parent_dir_id);

    return oi_copy;
}


void object_info_destroy(ListElement object_info){
    free(((Object_Info)(object_info))->object_id);
    free(((Object_Info)(object_info))->parent_dir_id);
    free(object_info);
}

/* ********************* END ********************* object_info Struct ********************* END ********************* */
/* ******************** START ******************** block_info Struct ******************** START ********************* */

ListElement copy_block_info(ListElement block_info , PMemory_pool mem_pool){
    assert(block_info);
    Block_Info bi = (Block_Info)(block_info);

    Block_Info bi_copy = memory_pool_alloc(mem_pool , sizeof(*bi_copy));
    if(bi_copy == NULL){
        return NULL;
    }

    bi_copy->size = bi->size;
    bi_copy->id = memory_pool_alloc(mem_pool , (sizeof(char)*(strlen(bi->id) +1)));
    if(bi_copy->id == NULL){
        return NULL; //All is allocated in POOL - Nothing to Free
    }
    strcpy(bi_copy->id , bi->id);

    return bi_copy;
}

void free_block_info(ListElement block_info){
    //All is allocated in POOL - Nothing to Free
    //Function exists for Compatibility to List Structure
    return;
}

/* ********************** END ********************* block_info struct ********************* END ********************* */

