//
// Created by Polina on 12-Mar-18.
//

/* **************************************************** INCLUDES **************************************************** */
#include "HashTable.h"
/* **************************************************** INCLUDES **************************************************** */
/* ****************************************************************************************************************** */
/* **************************************************** DEFINES ***************************************************** */

/* **************************************************** DEFINES ***************************************************** */
/* ******************** START ******************** HashTable Functions ******************** START ******************* */
HashTable ht_create(char type , PMemory_pool mem_pool) {
    HashTable ht = NULL;
    ht = memory_pool_alloc(mem_pool , sizeof(*ht)); //Allocate the table
    if(!ht){ //check allocation was successful
        return NULL;
    }
    switch(type){
        case 'B': //Blocks
            ht->size_table = BLOCKS_INIT_SIZE;
            break;
        case 'F': //Logical Files
            ht->size_table = FILES_INIT_SIZE;
            break;
        case 'D': //Directories
            ht->size_table = DIRS_INIT_SIZE;
            break;
        default:
            ht->size_table = INIT_SIZE_1; //Shouldn't really get here
            break;
    }

    /* Allocate pointers to the head nodes */
    ht -> table = memory_pool_alloc(mem_pool , (sizeof(Entry)*(ht->size_table)));
    if(!ht -> table ){ //check array od pointers was allocated successfully
        return NULL; //All is allocated in POOL - Nothing to Free
    }

    for(int i = 0; i < (ht->size_table) ; i++ ){
        ht->table[i] = NULL;
    }
    return ht;
}

unsigned long int ht_hash( HashTable ht, char *key ) {
    unsigned long int hashval = 0;
    int i = 0;
    /* Convert our string to an integer */
    while((hashval < ULONG_MAX) && (i < strlen(key))){
        hashval = hashval << 8;
        hashval += key[i];
        i++;
    }

    return (hashval % (ht->size_table));
}

Entry ht_newpair(char *key, unsigned int depth , unsigned long sn , unsigned int size , char flag ,
                 unsigned long physical_sn , char dedup_type , char* parent_dir_id , PMemory_pool mem_pool){

    Entry newpair  = memory_pool_alloc(mem_pool , sizeof(*newpair));
    if(newpair == NULL){
        return NULL;
    }

    newpair->key = memory_pool_alloc(mem_pool , (sizeof(char)*(strlen(key)+1)));

    if(newpair->key == NULL){
        return NULL; //All is allocated in POOL - Nothing to Free
    }
    newpair->key = strcpy(newpair->key , key);

    if(flag == 'B'){ // save the data object
        newpair->data = block_create(key , sn, size , mem_pool);
    }else if( flag == 'D'){
        newpair->data = dir_create(key , depth , sn , parent_dir_id , mem_pool);
    } else if(flag == 'F'){ //This is a file object
        newpair->data = file_create(key , depth , sn , size , physical_sn , dedup_type , parent_dir_id , mem_pool);
    }

    if(newpair->data == NULL) {
        return NULL; //All is allocated in POOL - Nothing to Free
    }

    newpair->next = NULL;
    return newpair;
}

Data ht_set(HashTable ht, char *key, unsigned int depth , unsigned long sn , unsigned int size , char flag,
            bool* object_exists , unsigned long physical_sn, char dedup_type, char* parent_dir_id ,
            PMemory_pool mem_pool) {
    Entry newpair = NULL;
    Entry next = NULL;
    Entry last = NULL;

    long int hash_key = ht_hash( ht , key );
    next = ht->table[hash_key];

    if(dedup_type == 'B' && flag == 'F'){// We are using Block Level Deduplication and we are working on File object
        newpair = ht_newpair(key, depth , sn, size, flag , physical_sn, dedup_type , parent_dir_id, mem_pool);
        newpair->next = next;
        ht->table[hash_key] = newpair;
        return newpair->data;
    }

    // The code gets here only in 2 cases:
    //              - Block Level Deduplication - Block object or Directory Object
    //              - File Level Deduplication - Block Object or Directory Object Since files are added differently without using ht_set
    /* Advance until get the end of the list OR first matching key */
    while( next != NULL && next->key != NULL && strcmp( key, next->key ) != 0 ) {
        last = next;
        next = next->next;
    }

    /* There's already a pair. Let's replace that string. */
    if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {
        //Return the pointer to the Block/File that already exists in the hash
        *object_exists = true;
        return next->data;
    } else { /* Nope, could't find it.  Time to grow a pair. */
        newpair = ht_newpair(key, depth , sn, size, flag , physical_sn , dedup_type , parent_dir_id , mem_pool); //allocate new pair
        if(newpair == NULL){
            return NULL;
        }
        /* We're at the start of the linked list in this hash_key. */
        if( next == ht->table[hash_key] ){ // If we in an empty list
            newpair->next = next;
            ht->table[hash_key] = newpair;

            /* We're at the end of the linked list in this hash_key. */
        } else if ( next == NULL ) {
            last->next = newpair;

        } else  { /* We're in the middle of the list. */
            newpair->next = next;
            last->next = newpair;
        }
        return newpair->data;
    }
}

Data ht_get(HashTable ht, char *key ) {
    long int hash_key = ht_hash(ht, key);
    Entry pair = ht->table[hash_key];

    /* Step through the hash_key, looking for our value. */
    while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) != 0 ) {
        pair = pair->next;
    }

    /* Did we actually find anything? */
    if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
        //didn't find anything
        return NULL;

    }
    //found the key - return the data
    return pair->data;
}

Data file_compare(HashTable ht_files , HashTable ht_physical_files ,
                  File file , File file_obj_p, unsigned long* physical_files_sn,
                  char dedup_type , PMemory_pool mem_pool){
    assert(file && file_obj_p);
    bool physical_file_exist = false , blocks_differ = false;
    Block_Info first_block = (Block_Info)listGetFirst(file->blocks_list);
    char* first_block_id = first_block->id;
    long int hash_key = ht_hash(ht_physical_files , first_block_id);

    /* ---------------------------------- Iterate over HT_PHYSICAL_FILES ---------------------------------- */
    File temp_file = NULL;
    Entry current = ht_physical_files->table[hash_key]; //get the cell in the hashtable for the possible file
    while(current != NULL && current->key != NULL){ //go over all files in the cell found above
        temp_file = ((File)(current->data));
        if(strcmp(file->object_id , temp_file->object_id) == 0){ //It's the same file
            current = current->next;
            continue;
        }
        if(file->file_size != temp_file->file_size){ //Compare by sizes
            current = current->next;
            continue;
        }
        if(file->num_blocks != temp_file->num_blocks){ //Compare by amount of blocks
            current = current->next;
            continue;
        }

        Object_Info temp_oi; //Compare each block
        Block_Info temp_file_blocks = listGetFirst(temp_file->blocks_list);
        LIST_FOREACH(Block_Info , iter ,file->blocks_list){
            if(strcmp(iter->id , temp_file_blocks->id) != 0){
                temp_oi = listGetFirst(temp_file->blocks_list);
                temp_oi = listGetFirst(file->blocks_list);
                blocks_differ = true; // the blocks aren't the same
                break;
            }
            temp_file_blocks = listGetNext(temp_file->blocks_list);
        }
        temp_oi = listGetFirst(temp_file->blocks_list);
        temp_oi = listGetFirst(file->blocks_list);
        if(blocks_differ == true){//advance to the next cell
            current = current->next;
            continue;
        } else { // We have found a match
            physical_file_exist = true;
            break;
        }
    } /* Finished searching for a physical file*/
    /* ---------------------------------- Iterate over HT_PHYSICAL_FILES ---------------------------------- */
    /* ---------------------------------------------------------------------------------------------------- */
    /* ---------------------------------- Adding the file to hash table ----------------------------------- */
    if(physical_file_exist == true) { // physical file already exits - add file to ht_files only
        file_set_logical_flag(file);
        listInsertLast_pool(temp_file->logical_files_list , &(file->object_sn) , mem_pool);
        (temp_file->num_files)++;
        file_set_physical_sn(file , temp_file->physical_sn); // set the physical sn of the logical file to be the one of the physical stored
        (*physical_files_sn)--;
        //file_destroy(file_obj_p , dedup_type);4-5-18
    } else { //add file only to ht_physical_files and to ht_files
        // hash by first block id
        hash_key = ht_hash(ht_physical_files , first_block_id);
        Entry ent = ht_physical_files->table[hash_key];

        Entry newpair  = memory_pool_alloc(mem_pool , sizeof(*newpair));
        assert(newpair);

        newpair->key = memory_pool_alloc(mem_pool , (sizeof(char)*(strlen(first_block_id) + 1)));
        assert(newpair->key);

        newpair->key = strcpy(newpair->key , first_block_id);
        newpair->data = file_obj_p;

        //Add the file in the head of the list
        newpair->next = ent;
        ht_physical_files->table[hash_key] = newpair;
    }
    /* -------------------- Adding the file to logical hash table anyway ----------------------------- */
    long int hash_key_f = ht_hash( ht_files , file->object_id );
    Entry curr = ht_files->table[hash_key_f];

    //Just add the logical file to the hashtable of logical files
    Entry newpair_l  = memory_pool_alloc(mem_pool , sizeof(*newpair_l));
    if(newpair_l == NULL){
        return NULL;
    }
    newpair_l->key = memory_pool_alloc(mem_pool , sizeof(char)*(strlen(file->object_id)+1));
    assert(newpair_l->key);
    newpair_l->key = strcpy(newpair_l->key , file->object_id);
    newpair_l->data = file;
    newpair_l->next = curr;
    ht_files->table[hash_key_f] = newpair_l;
    /* -------------------- Adding the file to logical hash table anyway ----------------------------- */
    return newpair_l->data;
}

/* ********************* END ********************* HashTable Functions ********************* END ******************** */

