//
// Created by Polina on 16-Mar-18.
//

#include "TextParsing.h"
/* ************************************************ Helper Functions ************************************************ */
void clear_line(char* line){
    if(strlen(line) >= 2){
        int len_buff = strcspn(line , "\r\n");
        line[len_buff] = '\n';
        line[len_buff + 1] = '\0';
    }
}

/* Compare between current buffer and string of "Z"*/
bool check_12_z(char buff[STR_OF_Z]){
    for (int i = 0; i < STR_OF_Z ; ++i) {
        if(buff[i] != 'z'){
            return false;
        }
    }
    return true;
}
/* ************************************************ Helper Functions ************************************************ */
/* ****************************************************************************************************************** */
/* *********************************************** Parsing Functions ************************************************ */
/* DIRECTORY NAME */
char* case_1_directory_name(char buff[BUFFER_SIZE]){
    //only first 10 digits depict the hashed directory name
    char* dir_name_hash = calloc(DIR_NAME_LEN,sizeof(char));
    if(!dir_name_hash){
        return NULL;
    }
    strncpy(dir_name_hash , buff , DIR_NAME_HASH);
    dir_name_hash[DIR_NAME_HASH] = '\0';
    return dir_name_hash;
}

/* NAMESPACE DEPTH */
unsigned short case_4_get_depth(char buff[BUFFER_SIZE]){
    unsigned short namespace_depth = (unsigned short)strtol(buff,(char**)NULL, 10);
    return namespace_depth ;
}

/* FILE SIZE */
unsigned int case_5_file_size(char buff[BUFFER_SIZE]){
    unsigned int file_size = (unsigned int)strtol(buff,(char **)NULL, 10);
    return file_size;
}

/* FILE ATTRIBUTES VALUE
 * Returns one from { 'D' , 'F'}
 *  @ 'D' - for directory
 *  @ 'F' - for file
 */
char case_6_file_attribute(char buff[BUFFER_SIZE]){
    unsigned int file_attribute = (unsigned int)strtol(buff,(char **)NULL, 16);
    char res;

    //Check for a directory, otherwise it is a file
    //The fifth bit should be set if this is a directory
    if((FILE_ATTRIBUTE_DIRECTORY & file_attribute) == FILE_ATTRIBUTE_DIRECTORY){
        res = 'D';
    } else{
        res = 'F';
    }
    return res;
}

/* FILE ID */
char* case_7_hash_file_id(char buff[BUFFER_SIZE], char* file_system_id){
    int text_len = strlen(buff) + 7;
    //FILE_ID_LEN
    char* file_id = calloc(text_len , sizeof(char)); // The value is 15 chars + 2 chars for index +1 for eol (end of line)
    if(file_id == NULL){
        return NULL;
    }
    //only first 15 digits depict the hashed directory name
    strcpy(file_id , file_system_id);
    strcat(file_id , buff);
    int id_len = strlen(file_id);
    file_id[id_len - 1] = '\0';
    return file_id;
}

/* Line 13 is SV */
// returns true if a file was created
void case_13_VS(FILE *input_file , char buff[BUFFER_SIZE] , int* block_line_count ,
                bool* read_empty_line_chucnks ,unsigned short depth, char* object_id,
                unsigned int file_size , bool* file_was_created, bool* finished_process_blocks ,
                PMemory_pool mem_pool,char dedup_type,
                HashTable ht_files , HashTable ht_blocks, HashTable ht_physical_files,
                unsigned long *files_sn , unsigned long *physical_files_sn, unsigned long *blocks_sn) {
    bool read_block = false;   // Params initialization
    bool object_exists = false;
    *read_empty_line_chucnks = false;
    char block_id[BLOCK_ID_LEN];
    unsigned int block_size = 0;
    Block new_block = NULL;

    while((read_block == false) && (*read_empty_line_chucnks == false)){
        switch ((int)(buff[0])) { //check next line
            case 'S':
                break;
            case 'V':
                break;
            case 'A':
                break;
            case LINE_SPACE :  // Empty line
                *read_empty_line_chucnks = true;
                break;
            default: //Data chunk
                read_block = true;
                break;
        }
        if((*read_empty_line_chucnks != true)&&(read_block != true)){
            fgets(buff, BUFFER_SIZE, input_file);
            clear_line(buff);
            (*block_line_count)++;
        }
    }
    if (*read_empty_line_chucnks == true) {
        return;
    }
    // If we got here it means we have blocks to read - Add file to files hashtable
    File file_obj = NULL , file_obj_p = NULL;
    if(dedup_type == 'B'){ //Block level deduplication
        file_obj = ht_set(ht_files , object_id , depth ,*files_sn , file_size ,'F',
                          &object_exists , *physical_files_sn,dedup_type , mem_pool);
    } else { // File level deduplication
        file_obj = file_create(object_id , depth , *files_sn , file_size , *physical_files_sn , dedup_type , mem_pool);
        file_obj_p = file_create(object_id , depth , *files_sn , file_size , *physical_files_sn, dedup_type , mem_pool);
    }
    (*files_sn)++; // logical_files_sn
    (*physical_files_sn)++;

    *file_was_created = true;
    object_exists = false;

    /* Read all data chunks  - Add Block Objects to hashtable*/
    if ((int)(buff[0]) != LINE_SPACE) {
        do { //we already have one chunk in the buffer
            char size[CHUNKE_SIZE_LEN] = "aaaaa";
            block_size = 0;
            if (check_12_z(buff) == true) { //only first 12 digits are block_id
                strncpy(block_id, buff, STR_OF_Z);
                block_id[STR_OF_Z] = '\0';
                strncpy(size, &buff[(STR_OF_Z + 1)], CHUNKE_SIZE_LEN-1);
            } else {
                //only first 10 digits are block_id
                strncpy(block_id, buff, 10);
                block_id[CHUNKE_ID_LEN] = '\0';
                strncpy(size, &buff[(CHUNKE_ID_LEN + 1)], CHUNKE_SIZE_LEN-1);
            }

            block_size = (int)strtol(size,NULL, 10);

            file_add_block(file_obj , block_id , block_size , mem_pool);
            if(dedup_type == 'F'){
                file_add_block(file_obj_p , block_id , block_size , mem_pool);
            }

            new_block = ht_set(ht_blocks , block_id , 1 , *blocks_sn , block_size , 'B', &object_exists , 0 , dedup_type , mem_pool);
            block_add_file(new_block , file_obj->object_id , mem_pool);

            if(object_exists == false){
                (*blocks_sn)++;
            }
            fgets(buff, BUFFER_SIZE, input_file);
            clear_line(buff);
            (*block_line_count)++;
            object_exists = false;
        } while (strlen(buff) > 1);
    }
    *finished_process_blocks = true;

    if(dedup_type == 'F'){ // Check if physical file already exists
        file_compare(ht_files ,ht_physical_files ,  file_obj , file_obj_p , physical_files_sn , dedup_type , mem_pool);
    }
    return;
}

//void update_parent_dir_sn(List previous , List current , int global_depth , int input_file_index ,
//                          PMemory_pool mem_pool , HashTable ht_files , HashTable ht_dirs , Dir* roots){
//    File temp_file = NULL;
//    Dir temp_dir = NULL;
//    Object_Info prev_list_iterator = NULL;
//    Object_Info curr_list_iterator = NULL;
//    int curr_level_objects_count = 0 , prev_list_size = 0 , curr_list_size = 0;
//
//    if(global_depth == 0){ //We are at root Level directory just set everyone to be the children of root
//        unsigned long root_sn = roots[input_file_index]->object_sn;
//        //Set root to be its own child
//        dir_set_parent_dir_sn(roots[input_file_index] , root_sn);
//        dir_add_sub_dir(roots[input_file_index] , root_sn , mem_pool);
//
//        LIST_FOREACH(Object_Info , iter ,current){
//            if(iter->object_type == 'F'){
//                temp_file = (File)(ht_get(ht_files , iter->object_id));
//                assert(temp_file);
//                file_set_parent_dir_sn(temp_file ,root_sn);
//                dir_add_file(roots[input_file_index],temp_file->object_sn , mem_pool);
//
//            } else{
//                temp_dir = (Dir)(ht_get(ht_dirs , iter->object_id));
//                assert(temp_dir);
//                dir_set_parent_dir_sn(temp_dir , root_sn);
//                dir_add_sub_dir(roots[input_file_index],temp_dir->object_sn , mem_pool);
//            }
//        }
//
//    }else{ //Go over both lists and update accordingly
//        prev_list_iterator = listGetFirst(previous);
//        curr_list_iterator = listGetFirst(current);
//        curr_level_objects_count = 0;
//        prev_list_size = listGetSize(previous);
//        curr_list_size = listGetSize(current);
//
//        for(int i = 0 ; i < prev_list_size ; i++){ //iterate over Previous level list
//            unsigned long current_sn_to_set = prev_list_iterator->object_sn;
//            Dir parent_dir_object = (Dir)(ht_get(ht_dirs , prev_list_iterator->object_id));
//
//            if(prev_list_iterator->object_type == 'F'){ //A File cant be Parent directory for anyone
//                prev_list_iterator = listGetNext(previous); //advance to the next object in the previous level
//                continue;
//            }else{ //The object is a directory it can be a parent
//                if(curr_level_objects_count >= curr_list_size){
//                    prev_list_iterator = listGetNext(previous); //advance to the next object in the previous level
//                    continue;
//                }
//                char* parent_id = curr_list_iterator->parent_dir_id;
//                //now lets iterate over the current list while we have objects with the same parent id
//                while((curr_list_iterator != NULL)&&(strcmp(parent_id , curr_list_iterator->parent_dir_id) == 0)){
//                    if(curr_list_iterator->object_type == 'F'){
//                        temp_file = (File)(ht_get(ht_files , curr_list_iterator->object_id));
//                        assert(temp_file);
//                        file_set_parent_dir_sn(temp_file ,current_sn_to_set);
//                        //add to the prevDir object - dir_add_file
//                        dir_add_file(parent_dir_object ,curr_list_iterator->object_sn , mem_pool);
//                    } else{
//                        temp_dir = (Dir)(ht_get(ht_dirs , curr_list_iterator->object_id));
//                        assert(temp_dir);
//                        dir_set_parent_dir_sn(temp_dir , current_sn_to_set);
//                        //add to the prevDir object - dir_add_sub_dir
//                        dir_add_sub_dir(parent_dir_object , curr_list_iterator->object_sn , mem_pool);
//                    }
//                    curr_list_iterator = listGetNext(current);//advance to the next object in the current level
//                    curr_level_objects_count++;
//                }
//            }
//            prev_list_iterator = listGetNext(previous); //advance to the next object in the previous level
//        }
//    }
//}

void update_parent_dir_sn(List previous , List current , int global_depth , int input_file_index ,
                          PMemory_pool mem_pool , HashTable ht_files , HashTable ht_dirs , Dir* roots){
    File temp_file = NULL;
    Dir temp_dir = NULL;
    Object_Info prev_list_iterator = NULL;
    Object_Info curr_list_iterator = NULL;
    int curr_level_objects_count = 0 , prev_list_size = 0 , curr_list_size = 0;

    if(global_depth == 0){ //We are at root Level directory just set everyone to be the children of root
        unsigned long root_sn = roots[input_file_index]->object_sn;
        //Set root to be its own child
        dir_set_parent_dir_sn(roots[input_file_index] , root_sn);
        dir_add_sub_dir(roots[input_file_index] , root_sn , mem_pool);

        LIST_FOREACH(Object_Info , iter ,current){
            if(iter->object_type == 'F'){
                temp_file = (File)(ht_get(ht_files , iter->object_id));
                assert(temp_file);
                file_set_parent_dir_sn(temp_file ,root_sn);
                dir_add_file(roots[input_file_index],temp_file->object_sn , mem_pool);

            } else{
                temp_dir = (Dir)(ht_get(ht_dirs , iter->object_id));
                assert(temp_dir);
                dir_set_parent_dir_sn(temp_dir , root_sn);
                dir_add_sub_dir(roots[input_file_index],temp_dir->object_sn , mem_pool);
            }
        }

    }else{ //Go over both lists and update accordingly
        prev_list_iterator = listGetFirst(previous);
        curr_list_iterator = listGetFirst(current);
        curr_level_objects_count = 0;
        prev_list_size = listGetSize(previous);
        curr_list_size = listGetSize(current);

        for(int i = 0 ; i < prev_list_size ; i++){ //iterate over Previous level list
            if(prev_list_iterator->object_type == 'F'){ //A File cant be Parent directory for anyone
                prev_list_iterator = listGetNext(previous); //advance to the next object in the previous level
                continue;
            }else{ //The object is a directory it can be a parent
                Dir parent_dir_object = (Dir)(ht_get(ht_dirs , prev_list_iterator->object_id));
                if(curr_level_objects_count >= curr_list_size){ //TODO - See if it can be return
                    prev_list_iterator = listGetNext(previous); //advance to the next object in the previous level
                    continue;
                }

                //now lets iterate over the current list while we have objects with the same parent id
                char* parent_id = curr_list_iterator->parent_dir_id;
                while((curr_list_iterator != NULL) && (strcmp(parent_id , curr_list_iterator->parent_dir_id) == 0)){
                    if(curr_list_iterator->object_type == 'F'){
                        temp_file = (File)(ht_get(ht_files , curr_list_iterator->object_id));
                        assert(temp_file);
                        file_set_parent_dir_sn(temp_file ,parent_dir_object->object_sn);
                        dir_add_file(parent_dir_object ,curr_list_iterator->object_sn , mem_pool); //add to the prevDir object - dir_add_file
                    } else{
                        temp_dir = (Dir)(ht_get(ht_dirs , curr_list_iterator->object_id));
                        assert(temp_dir);
                        dir_set_parent_dir_sn(temp_dir , parent_dir_object->object_sn);
                        dir_add_sub_dir(parent_dir_object , curr_list_iterator->object_sn , mem_pool); //add to the prevDir object - dir_add_sub_dir
                    }
                    curr_list_iterator = listGetNext(current);//advance to the next object in the current level
                    curr_level_objects_count++;
                }
                prev_list_iterator = listGetNext(previous); //advance to the next object in the previous level
            }
        }
    }
}

/* *********************************************** Parsing Functions ************************************************ */
void print_ht_to_CSV(char dedup_type , char** files_to_read, int num_of_input_files ,
                     unsigned long blocks_sn, unsigned long files_sn, unsigned long dir_sn , unsigned long physical_files_sn,
                     HashTable ht_files , HashTable ht_blocks, HashTable ht_dirs, HashTable ht_physical_files){
    Entry pair = NULL;
    File temp_file = NULL;
    Block temp_block = NULL;
    Dir temp_dir = NULL;
    FILE *results_file = NULL;
    char* fileName = malloc(350*sizeof(char));
    fileName = strcpy(fileName , "Parsing_Results_");

    for(int i = 0 ; i < num_of_input_files ; i++){
        char file_proc[5];
        strncpy(file_proc , files_to_read[i] , 4);
        file_proc[4] = '\0';
        fileName = strcat(fileName, file_proc);
        if(i < (num_of_input_files -1)){
            fileName = strcat(fileName , "_");
        }
    }
    if( dedup_type == 'B'){
        fileName = strcat(fileName , "_B.csv");
    } else {
        fileName = strcat(fileName , "_F.csv");
    }

    // Open the output file
    results_file = fopen(fileName , "w+");
    if(results_file == NULL){
        printf("Results file can not be opened - please try again \n");
        free(fileName);
        return;
    }

    if(dedup_type == 'B'){
        fprintf(results_file ,"# Output type: block-level\n");
    } else {
        fprintf(results_file ,"# Output type: file-level\n");
    }

    fprintf(results_file ,"# Input files: ");
    for(int i =0 ; i < num_of_input_files ; i++){
        if(i == num_of_input_files - 1){
            fprintf(results_file ,"%s" , files_to_read[i]);
        } else{
            fprintf(results_file ,"%s," , files_to_read[i]);
        }

    }
    fprintf(results_file ,"\n");

    fprintf(results_file ,"# Num files: %lu\n" , (files_sn));
    fprintf(results_file ,"# Num directories: %lu\n" , (dir_sn));
    if(dedup_type == 'B'){
        fprintf(results_file ,"# Num Blocks: %lu\n", (blocks_sn));
    } else {
        fprintf(results_file ,"# Num physical files: %lu\n", (physical_files_sn));
    }

    if(dedup_type == 'B'){ //Block level deduplication
        //Print Files - Logical
        for(int i = 0 ; i < (ht_files->size_table) ;i++){
            pair = ht_files->table[i];
            while( pair != NULL && pair->key != NULL) {
                temp_file = ((File)(pair->data));
                fprintf(results_file , "F,%lu,%s,%lu,%d,",
                        temp_file->object_sn, temp_file->object_id , temp_file->parent_dir_sn,
                        temp_file->num_blocks);
                //Object_Info temp_oi;
                LIST_FOREACH(Block_Info , iter ,temp_file->blocks_list){
                    unsigned long block_sn = ((Block)(ht_get(ht_blocks , iter->id)))->block_sn;
                    fprintf(results_file ,"%lu,%d," , block_sn , iter->size);
                }
                //temp_oi = listGetFirst(temp_file->blocks_list);
                fprintf(results_file ,"\n");
                pair = pair->next;
            }
        }
        //Print Blocks
        for(int i = 0 ; i < (ht_blocks->size_table) ;i++){
            pair = ht_blocks->table[i];
            while( pair != NULL && pair->key != NULL) {
                temp_block = ((Block)(pair->data));
                fprintf(results_file , "B,%lu,%s,%d,",
                        temp_block->block_sn , temp_block->block_id,
                        temp_block->shared_by_num_files);
                for(int j = 0 ; j < (temp_block->files_ht->size_table) ; j++){
                    EntryF pair_file_id = temp_block->files_ht->table[j];
                    while( pair_file_id != NULL && pair_file_id->key != NULL) {
                        unsigned long file_sn = ((File)(ht_get(ht_files , pair_file_id->key)))->object_sn;
                        fprintf(results_file ,"%lu," , file_sn);
                        pair_file_id = pair_file_id->next;
                    }
                }
                fprintf(results_file ,"\n");
                pair = pair->next;
            }
        }
    }else{//Print logical files
        for(int i = 0 ; i < (ht_files->size_table) ;i++){
            pair = ht_files->table[i];
            while( pair != NULL && pair->key != NULL) {
                temp_file = ((File)(pair->data));
                fprintf(results_file , "F,%lu,%s,%lu,%d,%lu,%d,\n",
                        temp_file->object_sn, temp_file->object_id , temp_file->parent_dir_sn,
                        1, temp_file->physical_sn, temp_file->file_size);
                pair = pair->next;
            }
        }

        //Print physical files
        for(int i = 0 ; i < (ht_physical_files->size_table) ;i++){
            pair = ht_physical_files->table[i];
            while( pair != NULL && pair->key != NULL) {
                temp_file = ((File)(pair->data));
                fprintf(results_file , "P,%lu,%s,%d,",
                        temp_file->physical_sn, temp_file->object_id ,
                        temp_file->num_files);
                LIST_FOREACH(unsigned long* , iter1 ,temp_file->logical_files_list){
                    fprintf(results_file ,"%lu," , *iter1);
                }
                fprintf(results_file ,"\n");
                pair = pair->next;
            }
        }
    }

    //Print Directories
    for(int i = 0 ; i < (ht_dirs->size_table) ;i++){
        pair = ht_dirs->table[i];
        while( pair != NULL && pair->key != NULL) {
            temp_dir = ((Dir)(pair->data));
            if(temp_dir->dir_depth == -1){
                fprintf(results_file , "R,");
            }else {
                fprintf(results_file , "D,");
            }
            fprintf(results_file , "%lu,%s,%lu,%d,%d," ,
                    temp_dir->object_sn, temp_dir->object_id, temp_dir->parent_dir_sn,
                    temp_dir->num_of_subdirs, temp_dir->num_of_files);
            LIST_FOREACH(unsigned long* , iter , temp_dir->dirs_list){
                fprintf(results_file ,"%lu," , *(iter));
            }
            LIST_FOREACH(unsigned long* , iter , temp_dir->files_list){
                fprintf(results_file ,"%lu," , *(iter));
            }
            fprintf(results_file , "\n");
            pair = pair->next;
        }
    }

    fclose(results_file);
    free(fileName);
}