/* *************************************************** INCLUDES ***************************************************** */
#include "Utilities.h"
#include "HashTable.h"
#include "TextParsing.h"

/* ************************************************ Global Params *************************************************** */
/* Serial number for counting the elements which insert to the system */
// files_sn is the logical sn-number
unsigned long blocks_sn = 0 , files_sn = 0 , dir_sn = 0, physical_files_sn = 0;

/* Hash-Tables for blocks, files , directories */
HashTable ht_files = NULL , ht_blocks = NULL , ht_dirs = NULL ,ht_physical_files = NULL;

/* Root Directory */
Dir* roots = NULL;
char dedup_type;
/* ************************************************ Global Params *************************************************** */
/* ********************************************************************************************************************/
/* ***************************************************** MAIN ******************************************************* */
int main(int argc , char** argv){
    // Allocate Initial Memory Pool
    PMemory_pool mem_pool = calloc(1 , sizeof(Memory_pool));
    memory_pool_init(mem_pool);

    /* ----------------------- Parameters Declarations & Initialization ----------------------- */
    /* Define Files to be read */
    int num_input_files = 0;
    char* current_working_directory = NULL;
    char** files_to_read = NULL;
    char srv_idx_first[FILE_SYSTEM_ID_LEN+2];
    char srv_idx_last[FILE_SYSTEM_ID_LEN+2];

    if(argc == 1){
        printf("No Extra Command Line Argument Passed Other Than Program Name\n");
        return 0;
    }
    dedup_type = argv[1][0];
    num_input_files = atoi(argv[2]);
    current_working_directory = (char*)memory_pool_alloc(mem_pool , (strlen(argv[3]) + 1)*sizeof(char));
    strcpy(current_working_directory , argv[3]);
    /* Read the rest of the line to get all file names */
    files_to_read = (char**)memory_pool_alloc(mem_pool , (num_input_files * sizeof(char*)));
    for(int i = 0 ; i < num_input_files ; i++){
        files_to_read[i] = (char*)memory_pool_alloc(mem_pool,(strlen(argv[4 + i]) + 1) * sizeof(char));
        strcpy(files_to_read[i] , argv[4 + i]);
    }
    roots = memory_pool_alloc(mem_pool, num_input_files* sizeof(*roots));

    // File  Manipulation Variables
    FILE *input_file = NULL;
    char buff[BUFFER_SIZE];
    bool read_empty_line_chucnks = false;
    bool finished_process_blocks = false;
    int chunk_line_count = 0;
    char* current_file = NULL;
    bool finished_reading_file = false;

    // Initialize Global Variables
    ht_files = ht_create('F' , mem_pool);
    ht_physical_files = ht_create('F' , mem_pool);
    ht_blocks = ht_create('B' , mem_pool);
    ht_dirs = ht_create('D' , mem_pool);


    if(ht_files == NULL || ht_blocks == NULL || ht_dirs == NULL ||ht_physical_files == NULL){
        printf("(Parser)--> Failed Allocating Hash Tables in parser =[ \n");
        return 0;
    }

    /* Define parameters for global data Manipulation (over entire input file) */
    List curr_depth_objects = NULL , previous_depth_objects = NULL;
    int curr_depth_objects_counter = 0;
    int global_current_depth = 0 ;
    curr_depth_objects = listCreate(dummy_copy_func , dummy_free_func);
    previous_depth_objects = listCreate(dummy_copy_func , dummy_free_func);
    char* curr_depth_objects_type = calloc(OBJECT_DEPTH_ARRAY_SIZE , sizeof(char));
    char* previous_depth_objects_type = calloc(OBJECT_DEPTH_ARRAY_SIZE , sizeof(char));
    assert(curr_depth_objects_type);
    assert(previous_depth_objects_type);

    /* Define parameters for reading data regarding SINGLE OBJECT */
    char file_system_ID[FILE_SYSTEM_ID_LEN+2];
    char* parent_dir_id = NULL; // Hashed ID of parent Directory
    unsigned short depth = 0; //Depth of current object in the hierarchy
    unsigned int file_size = 0; //File Size (if the object is a file)
    char obj_type = 'Z'; //Hexadecimal Value - Tells the type of the object ( A combination of binary flags set)
    char* object_id = NULL; //Hashed ID of the current object
    bool is_zero_size_file = false;
    bool file_was_created = false;
    bool object_exists_in_hash_already = false;
    bool set_root = false;


    Dir current_dir_object = NULL;
    File current_file_object = NULL;

    /* ----------------------- Parameters Declarations & Initialization ----------------------- */
    /* ---------------------------------------------------------------------------------------- */
    /* ------------------------------------- File Reading ------------------------------------- */
    /* Go over all file systems */
    for (int i = 0; i < num_input_files ; ++i) { /* (1) Read an Input File */
        current_file = calloc((strlen(current_working_directory) + strlen(files_to_read[i]) + 1) , sizeof(char)); //Temporary Allocation - Do not allocate in POOL !!!!!
        strcpy(current_file , current_working_directory);
        strcat(current_file , files_to_read[i]);
        input_file = fopen(current_file , "r");
        if(input_file == NULL){ //check the file was opened successfully - if not terminate
            printf("(Parser)--> Can't open input file/s =[ \n");
            return 0;
        }
        free(current_file);

        fgets(buff, BUFFER_SIZE , input_file); //Read First Line
        clear_line(buff);
        fgets(buff, BUFFER_SIZE , input_file); //Read Second Line
        clear_line(buff);
        fgets(buff, BUFFER_SIZE , input_file); //READFile System ID - get last 3 digits
        clear_line(buff);
        strncpy(file_system_ID , buff + 9 , 3);
        file_system_ID[FILE_SYSTEM_ID_LEN]='_';
        file_system_ID[FILE_SYSTEM_ID_LEN + 1]='\0';
        int dec_idx;
        char temp_id[FILE_SYSTEM_ID_LEN + 1];
        if(i == 0){
            strncpy(temp_id , buff + 9 , 3);
            temp_id[FILE_SYSTEM_ID_LEN] = '\0';
            //convert to decimal
            dec_idx = (int)strtol(temp_id , NULL , 16);
            sprintf(srv_idx_first , "%03d",dec_idx);
        }
        if(i == num_input_files - 1){
            strncpy(temp_id , buff + 9 , 3);
            temp_id[FILE_SYSTEM_ID_LEN] = '\0';
            // convert to decimal
            dec_idx = (int)strtol(temp_id , NULL , 16);
            sprintf(srv_idx_last , "%03d",dec_idx);
        }

        /* Skip till the first empty line - over the file system description */
        do{
            fgets(buff, BUFFER_SIZE , input_file);
            clear_line(buff);
        } while(strlen(buff) > 1);

        set_root = true;

        /* Read File till the end - parse each block and add it to the corresponding structure */
        while(!feof(input_file)){
            fgets(buff, BUFFER_SIZE , input_file);
            clear_line(buff);
            chunk_line_count++;
            /* Check if we have reached the end of the file, nothing more to read */
            if((strcmp(buff , "LOGCOMPLETE\r\n") == 0) || (strcmp(buff , "LOGCOMPLETE\n") == 0)){
                /* Read the last line before EOF */
                fgets(buff, BUFFER_SIZE , input_file);
                clear_line(buff);
                finished_reading_file = true;
            }

            /* We haven't seen the LOGCOMPLETE line yet */
            /* Check if we haven't reached the end of the current input block */
            /***********************************************************************************************/
            while (strlen(buff) > 1 && !feof(input_file)){ /* Processing Object */
                switch(chunk_line_count){
                    case 1: /* get DIRECTORY NAME */
                        parent_dir_id = case_1_directory_name(buff);
                        break;
                    case 4: /* get NAMESPACE DEPTH */
                        depth = case_4_get_depth(buff);
                        //Check if current depth (in variable depth) is bigger than the one in global_current_depth
                        if(depth > global_current_depth){
                            //This means we have reached a new depth and can update parent_dir_sn for objects from previous levels
                            update_parent_dir_sn(previous_depth_objects , curr_depth_objects , global_current_depth ,
                                                 i , mem_pool , roots,
                                                 curr_depth_objects_type , previous_depth_objects_type);
                            //Update Object lists
                            listDestroy(previous_depth_objects); //Empty the previous_depth_objects list
                            previous_depth_objects = listCopy(curr_depth_objects);//Copy the curr_depth_objects list to the previous_depth_objects
                            memcpy(previous_depth_objects_type , curr_depth_objects_type ,sizeof(char)*OBJECT_DEPTH_ARRAY_SIZE);
                            listClear(curr_depth_objects); //Empty the curr_depth_objects list
                            memset(curr_depth_objects_type, 0 , sizeof(char)*OBJECT_DEPTH_ARRAY_SIZE);
                            curr_depth_objects_counter = 0;
                            global_current_depth = depth;
                        }
                        break;
                    case 5: /* get FILE SIZE */
                        file_size = case_5_file_size(buff);
                        break;
                    case 6: /* get FILE ATTRIBUTES VALUE */
                        obj_type = case_6_file_attribute(buff);
                        if( ( file_size == 0 ) && ( obj_type == 'F' ) ){ // ignore zero size files
                            is_zero_size_file = true;
                        }
                        break;
                    case 7: /* get OBJECT ID */
                        object_id = case_7_hash_file_id(buff , file_system_ID);
                        //Adding Directory Object to HashTable
                        if(obj_type == 'D'){
                            if( dir_sn == 0 || set_root == true){ //Creating Dummy Root Node using the Parent_dir_id of the first object in the input file
                                char* root_id = calloc(ROOT_ID_LEN , sizeof(char)); // The value is 8 chars
                                if(root_id == NULL){
                                    printf("(Parser) --> Can't Allocate place for ROOT_ID \n");
                                    break;
                                }
                                strcpy(root_id , file_system_ID);
                                strcat(root_id , "root");
                                root_id[8] = '\0';
                                roots[i] = ht_set(ht_dirs , root_id , -1 , dir_sn ,DIR_SIZE , 'D' , &object_exists_in_hash_already , 0 , dedup_type , parent_dir_id ,mem_pool);
                                current_dir_object = roots[i];
                                dir_sn++;
                                free(root_id);
                                set_root = false;
                            }
                            //Create Directory Object with the retrieved data
                            current_dir_object = ht_set(ht_dirs, object_id, depth, dir_sn, DIR_SIZE , 'D' , &object_exists_in_hash_already , 0, dedup_type , parent_dir_id , mem_pool);
                            dir_sn++;
                        }
                        break;
                    case 13: /* Line 13 is SV */
                        current_file_object = case_13_VS(input_file , buff , &chunk_line_count ,
                                                         &read_empty_line_chucnks , depth , object_id,file_size,
                                                         &file_was_created, &finished_process_blocks , mem_pool,dedup_type,
                                                         ht_files , ht_blocks, ht_physical_files,
                                                         &files_sn , &physical_files_sn, &blocks_sn, parent_dir_id);
                        // Add object (File or Directory) to curr_depth_objects list
                        if ((obj_type == 'F') && (is_zero_size_file == false) && (file_was_created == true)){
                            if(current_file_object != NULL){
                                listInsertLast(curr_depth_objects , current_file_object);
                                curr_depth_objects_type[curr_depth_objects_counter] = 'F';
                                curr_depth_objects_counter++;
                                assert(!(curr_depth_objects_counter > OBJECT_DEPTH_ARRAY_SIZE));
                            }
                            //object_info_destroy(oi_file); //The list adds a copy of this object and it is no longer needed
                        } else if(obj_type == 'D'){ //Adding Directory Object to HashTable
                            //add directory to curr_depth_objects list in order to later find the parent directory
                            listInsertLast(curr_depth_objects , current_dir_object);
                            curr_depth_objects_type[curr_depth_objects_counter] = 'D';
                            curr_depth_objects_counter++;
                            assert(!(curr_depth_objects_counter > OBJECT_DEPTH_ARRAY_SIZE));
                        }
                        break;
                    default:
                        break;
                }

                if(read_empty_line_chucnks == false && finished_process_blocks == false){
                    fgets(buff, BUFFER_SIZE , input_file); //read next line in current block
                    clear_line(buff);
                    chunk_line_count++;
                }
            } /* Processing Object */

            /***********************************************************************************************/
            /******************* WE HAVE REACHED THE END OF THE CURRENT INPUT OBJECT !!! ********************/
            if(!feof(input_file)){ //Update parameters that are relevant to a single object
                chunk_line_count = 0; /* Zero the line count for the next block */
                read_empty_line_chucnks = false;
                is_zero_size_file = false;
                free(parent_dir_id); //stays with regular malloc
                free(object_id);//stays with regular malloc
                file_was_created = false;
                finished_process_blocks = false;
                parent_dir_id = NULL; // Hashed ID of parent Directory
                object_id = NULL;
                object_exists_in_hash_already = false;
            }
        }
        fclose(input_file);
        free(parent_dir_id);

        //This means we have reached a new depth and can update parent_dir_sn for objects from previous levels
        update_parent_dir_sn(previous_depth_objects , curr_depth_objects , global_current_depth ,
                             i , mem_pool , roots, curr_depth_objects_type , previous_depth_objects_type);
        if(finished_reading_file == true){
            global_current_depth = 0;
            listClear(curr_depth_objects); //Empty the curr_depth_objects list
            memset(curr_depth_objects_type, 0 , sizeof(char)*OBJECT_DEPTH_ARRAY_SIZE);
            listClear(previous_depth_objects); //Empty the curr_depth_objects list
            memset(previous_depth_objects_type, 0 , sizeof(char)*OBJECT_DEPTH_ARRAY_SIZE);
            curr_depth_objects_counter = 0;
            chunk_line_count = 0; /* Zero the line count for the next block */
            read_empty_line_chucnks = false;
            is_zero_size_file = false;
            file_was_created = false;
            finished_process_blocks = false;
            object_exists_in_hash_already = false;
            finished_reading_file = false;
        }
    } /* (1) Read an Input File */

    print_ht_to_CSV(dedup_type, files_to_read , num_input_files,
                    blocks_sn, files_sn, dir_sn, physical_files_sn,
                    ht_files, ht_blocks, ht_dirs, ht_physical_files, srv_idx_first, srv_idx_last);

    //Free All Hash tables and Lists
    listDestroy(previous_depth_objects);
    listDestroy(curr_depth_objects);
    free(curr_depth_objects_type);
    free(previous_depth_objects_type);
    memory_pool_destroy(mem_pool);
    free(mem_pool);

    return 0;
}
