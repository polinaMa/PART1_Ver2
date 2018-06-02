//
// Created by mihush on 11/12/2017.
//

#ifndef DEDUPLICATION_PROJ_HASHTABLE_H
#define DEDUPLICATION_PROJ_HASHTABLE_H
/* **************************************************** INCLUDES **************************************************** */
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "Block.h"
#include "File.h"
#include "List.h"
#include "Directory.h"
/* **************************************************** INCLUDES **************************************************** */
/* ****************************************************************************************************************** */
/* **************************************************** DEFINES ***************************************************** */
#define INIT_SIZE_1 5007
#define BLOCKS_INIT_SIZE 2000000
#define FILES_INIT_SIZE 2000000
#define DIRS_INIT_SIZE 1000000
typedef void* Data;
/* **************************************************** DEFINES ***************************************************** */
/* ****************************************************************************************************************** */
/* ******************* START ******************** HashTable Definition ******************** START ******************* */

struct entry_t {
    char *key;
    Data data;
    struct entry_t *next;//Chain-hashing solution. ptr to the next element
};
typedef struct entry_t *Entry;

struct hashtable_t {
    long size_table;
    long num_of_elements;
    Entry *table; // array of pointers to Entries
};
typedef struct hashtable_t *HashTable;

/* ********************* END ******************** HashTable Definition********************* END ********************* */
/* ****************************************************************************************************************** */
/* ****************************************************************************************************************** */
/* ******************** START ******************** HashTable Functions ******************** START ******************* */

/*
 * ht_create - creates a hashtable for the requested type of objects (which determines its size)
 *
 * @type - can be one of 3 : 'B' for blocks , 'F' for files and 'D' for directories
 */
HashTable ht_create(char type , PMemory_pool mem_pool);

/*
 * ht_hash - Given a key (string) Generates a Hash Value by which it will be stored in the table
 *
 * @ht  - the hashtable in which the key will be saved
 * @key - the key for which we want to get the hashed value in the hashtable
 */
unsigned long int ht_hash( HashTable ht, char *key );

/*
 * ht_newpair - Creates a key-value pair
 *               - For block - size parameter will contain the block size
 *               - For File - size parameter will be -1
 */
Entry ht_newpair(char *key, unsigned int depth , unsigned long sn , unsigned int size , char flag ,
                 unsigned long physical_sn , char dedup_type , PMemory_pool mem_pool);

/*
 * ht_set - Insert a key-value pair into a hash table (General function thus
 *
 * @ht            - the hashtable to which the object will be added
 * @key           - the hashed id of the object
 * @depth         - the depth of the object in the filesystem hierarchy
 * @sn            - the serial number of the object
 * @size          - the size of the object
 * @flag          - a flag that signifies the object: 'B' for block , 'F' for File and 'D' for directory
 * @object_exists - pointer to variable that will be an output of the function - True if the block or file already exist in the hashtable
 * @physical_sn   - the serial number of a physical file
 */
Data ht_set(HashTable ht, char *key, unsigned int depth , unsigned long sn , unsigned int size , char flag,
            bool* object_exists , unsigned long physical_sn, char dedup_type , PMemory_pool mem_pool);

/*
 * ht_get - Retrieve pointer for block/file element with corresponding key in hash table
 *
 * @ht  - the hashtable to which the object will be added
 * @key - the hashed id of the object
 */
Data ht_get( HashTable ht, char *key );

/*
 * hashTable_destroy - Freeing all allocations of HashTable
 *
 * @ht - hashtable to destroy
 * @flag - flag that signifies if the object is flag ('F') or block 'B' or directory 'D'
 */
void hashTable_destroy(HashTable ht , char flag ,  char dedup_type);

/*
 * file_compare_to_File - files are considered identical if have the same blocks
 *                          1 - check sizes
 *                          2 - check amount of blocks
 *                          3 - check first block , second block, etc ....
 *                         returns false if physical file already exists
 *                         returns true if no physical file exists
 */
Data file_compare(HashTable ht_files , HashTable ht_physical_files ,
                  File file , File file_obj_p, unsigned long* physical_files_sn,
                  char dedup_type , PMemory_pool mem_pool);

/* ********************* END ********************* HashTable Functions ********************* END ******************** */

#endif //DEDUPLICATION_PROJ_HASHTABLE_H
