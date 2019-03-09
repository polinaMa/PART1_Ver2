//
// Created by Polina on 16-Mar-18.
//

#ifndef DEDUPLICATIONPROJECT_TEXTPARSING_H
#define DEDUPLICATIONPROJECT_TEXTPARSING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "HashTable.h"
#include "Utilities.h"

/*
 * clear_line - receives a line in dos format (New line is represented by \r\n
 *              and converts it to Unix format (only \n for New Line)
 *
 * @line - Pointer to the line to be cleared.
 */
void clear_line(char* line);

/*
 * check_12_z - Recieves a string and determines if it is 12 Z's, if yes returns true.
 *              Otherwise, returns False.
 *
 * @buff - buffer containing the string
 */
bool check_12_z(char buff[STR_OF_Z]);

/*
 * case_1_directory_name - Extracts the Directory hashed id from the string received as input.
 *                         (The string contains the relevant line from the input text file).
 *
 * @buff - input string
 */
char* case_1_directory_name(char buff[BUFFER_SIZE]);

/*
 * case_4_get_depth - Extracts the depth of the object from the string received as input.
 *                    (The string contains the relevant line from the input text file).
 *
 * @buff - input string
 */
unsigned short case_4_get_depth(char buff[BUFFER_SIZE]);

/*
 * case_5_file_size - Extracts the file size from the string received as input.
 *                    (The string contains the relevant line from the input text file).
 *
 * @buff - input string
 */
unsigned int case_5_file_size(char buff[BUFFER_SIZE]);

/*
 * case_6_file_attribute - Extracts the type of object from the string received as input.
 *                         (The string contains the relevant line from the input text file).
 *                         It returns:
 *                              - F for file
 *                              - D for Directory
 * @buff - input string
 */
char case_6_file_attribute(char buff[BUFFER_SIZE]);

/*
 * case_7_hash_file_id - Extracts theid of the object(file or directory) from the string received as input.
 *                       (The string contains the relevant line from the input text file).
 *                       The file system id received as input is concatenated with the
 *                       extracted hashed id and returned as output.
 * @buff           - input string
 * @file_system_id - string containing the file system id the object belongs to
 */
char* case_7_hash_file_id(char buff[BUFFER_SIZE], char* file_system_id);

/*
 * case_13_VS - Creates the file objects according to the data parsed from the input file
 *              and saves the files and blocks to relevant hash tables
 *
 * @input_file              - Pointer to the file object being currently read as input
 * @buff                    - input string - buffer for reading each line
 * @block_line_count        - Pointer to a Counter for the number of lines we have read from the currently
 *                            processed file object from the input_file
 * @read_empty_line_chucnks - boolean variable indicating we have reached the end of the file object being
 *                            processed from the input file
 * @depth                   - current depth of file being processed
 * @object_id               - the id of the file object currently being processed
 * @file_size               - the size of the file object currently being processed
 * @file_was_created        - pointer to boolean variable indicating that a file object was created in the hash table
 * @finished_process_blocks - pointer to boolean variable indicating we have read all the blocks of the currently
 *                            processed file object
 * @mem_pool                - pointer to memory pool structure
 * @dedup_type              - deduplication type, F for file-level and B for block-level
 * @ht_files                - Pointer to hash table of logical file objects
 * @ht_blocks               - Pointer to hash table of block objects
 * @ht_physical_files       - Pointer to hash table of physical file objects
 * @files_sn                - Pointer to serial number counter fo logical file objects
 * @physical_files_sn       - Pointer to serial number counter fo physical file objects
 * @blocks_sn               - Pointer to serial number counter fo block objects
 */
File case_13_VS(FILE *input_file , char buff[BUFFER_SIZE] , int* chunk_line_count ,
                bool* read_empty_line_chucnks ,unsigned short depth, char* object_id,
                unsigned int file_size , bool* file_was_created, bool* finished_process_blocks ,
                PMemory_pool mem_pool,char dedup_type,
                HashTable ht_files , HashTable ht_blocks, HashTable ht_physical_files,
                unsigned long *files_sn , unsigned long *physical_files_sn, unsigned long *blocks_sn,
                char* parent_dir_id, int blocks_filter_param_k);
/*
 * update_parent_dir_sn - receives a list of descriptors of objects from current depth and previous depth
 *                        and updates the file hierarchy
 *
 * @previous         - List of descriptors of files and directories from previous level
 * @current          - List of descriptors of files and directories from current level
 * @global_depth     - the depth currently being processed
 * @curr_root_index  - index of the currently processed input file in order to find the correct root object
 * @mem_pool         - pointer to memory pool structure
 * @ht_files         - Pointer to hash table of logical file objects
 * @ht_dirs          - Pointer to hash table of directory objects
 * @roots            - Pointer to array of root directories
 */
void update_parent_dir_sn(List previous , List current , int global_depth , int curr_root_index ,
                          PMemory_pool mem_pool , Dir* roots, char* curr_depth_objects_type , char* previous_depth_objects_type);

/*
 * print_ht_to_CSV - Print into a csv file that starts with "Parsing_Results.." the data about
 *                   all objects in the code.
 * @dedup_type         - deduplication type, F for file-level and B for block-level
 * @files_to_read      - string of file names that were parsed
 * @num_of_input_files - number of files that were parsed
 * @blocks_sn          - serial number counter fo block objects
 * @files_sn           - serial number counter fo logical file objects
 * @dir_sn             - serial number counter fo directory objects
 * @physical_files_sn  - serial number counter fo physical file objects
 * @ht_files           - Pointer to hash table of logical file objects
 * @ht_blocks          - Pointer to hash table of block objects
 * @ht_dirs            - Pointer to hash table of directory objects
 * @ht_physical_files  - Pointer to hash table of physical file objects
 */
void print_ht_to_CSV(char dedup_type , char** files_to_read, int num_of_input_files ,
                     unsigned long blocks_sn, unsigned long files_sn, unsigned long dir_sn , unsigned long physical_files_sn,
                     HashTable ht_files , HashTable ht_blocks, HashTable ht_dirs, HashTable ht_physical_files,
                     char* srv_idx_first, char* srv_idx_last, int blocks_filter_param_k);

/*
 * blocks_filter_rule - function which returns if a block answer a threshold number
 *                      of 0-s in the most significant bits
 * @blocks_filter_param_k   - the threshold parameter, k
 * @id                      - the block id
 * return:              ---> true - if there are at lest k 0-s bits in the most significant bits
 *                      ---> false - otherwise
 */
bool blocks_filter_rule(int blocks_filter_param_k, char* id);

bool ascii_to_binary(char *input,  char **value, int len, int blocks_filter_param_k);


#endif //DEDUPLICATIONPROJECT_TEXTPARSING_H