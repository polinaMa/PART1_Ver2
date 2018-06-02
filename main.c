/* *************************************************** INCLUDES ***************************************************** */
#include "Utilities.h"
#include "HashTable.h"
#include "TextParsing.h"
#include <time.h>

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
    PMemory_pool mem_pool = calloc(1 , sizeof(Memory_pool));
    memory_pool_init(mem_pool);

    FILE* monitor_file = fopen("Monitor.txt" , "w+");
    time_t rawtime;
    struct tm *timeinfo;
    /* ----------------------- Parameters Declarations & Initialization ----------------------- */
    /* Define Files to be read */
    int num_input_files = 0;
    char* current_working_directory = NULL;
    char** files_to_read = NULL;
    if(argc == 1){
        printf("No Extra Command Line Argument Passed Other Than Program Name\n");
        return 0;
    }

    dedup_type = argv[1][0];
    printf("%c\n" , dedup_type);

    num_input_files = atoi(argv[2]);
    printf("%d\n" , num_input_files);

    current_working_directory = (char*)memory_pool_alloc(mem_pool , (strlen(argv[3]) + 1)*sizeof(char));
    strcpy(current_working_directory , argv[3]);
    printf("%s\n" , current_working_directory);

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
    int block_line_count = 0;
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
    int global_current_depth = 0 ;
    curr_depth_objects = listCreate(object_info_copy , object_info_destroy);
    previous_depth_objects = listCreate(object_info_copy , object_info_destroy);

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

    /* ----------------------- Parameters Declarations & Initialization ----------------------- */
    /* ---------------------------------------------------------------------------------------- */
    /* ------------------------------------- File Reading ------------------------------------- */
    /* Go over all file systems */
    for (int i = 0; i < num_input_files ; ++i) { /* (1) Read an Input File */
        current_file = calloc((strlen(current_working_directory) + strlen(files_to_read[i]) + 1) , sizeof(char)); //Temporary Allocation - Do not allocate in POOL !!!!!
        strcpy(current_file , current_working_directory);
        strcat(current_file , files_to_read[i]);
        fprintf(monitor_file, "(Parser)--> ----- Opening File %s ----- \n" , current_file);
        printf("(Parser)--> ----- Opening File %s ----- \n" , current_file);
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        fprintf(monitor_file, "(Parser)-->              %s             \n" , asctime (timeinfo));
        fflush(monitor_file);
        input_file = fopen(current_file , "r");
        if(input_file == NULL){ //check the file was opened successfully - if not terminate
            printf("(Parser)--> Can't open input file/s =[ \n");
            return 0;
        }
        free(current_file);
        fprintf(monitor_file, "(Parser)-->  ----- Start Reading the file ----- \n");
        printf("(Parser)-->  ----- Start Reading the file ----- \n");
        fgets(buff, BUFFER_SIZE , input_file); //Read First Line
        clear_line(buff);
        fgets(buff, BUFFER_SIZE , input_file); //Read Second Line
        clear_line(buff);
        fgets(buff, BUFFER_SIZE , input_file); //READFile System ID - get last 3 digits
        clear_line(buff);
        strncpy(file_system_ID , buff + 9 , 3);
        file_system_ID[FILE_SYSTEM_ID_LEN]='_';
        file_system_ID[FILE_SYSTEM_ID_LEN + 1]='\0';

        /* Skip till the first empty line - over the file system description */
        do{
            fgets(buff, BUFFER_SIZE , input_file);
            clear_line(buff);
        } while(strlen(buff) > 1);

        set_root = true;
        fprintf(monitor_file, "(Parser)--> --- Skipped over the file-system data block successfully--- \n");
        printf("(Parser)--> --- Skipped over the file-system data block successfully--- \n");

        /* Read File till the end - parse each block and add it to the corresponding structure */
        while(!feof(input_file)){
            fgets(buff, BUFFER_SIZE , input_file);
            clear_line(buff);
            block_line_count++;
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
                switch(block_line_count){
                    case 1: /* DIRECTORY NAME */
                        parent_dir_id = case_1_directory_name(buff);
                        break;
                    case 4: /* NAMESPACE DEPTH */
                        depth = case_4_get_depth(buff);
                        //Check if current depth (in variable depth) is bigger than the one in global_current_depth
                        if(depth > global_current_depth){
                            //This means we have reached a new depth and can update parent_dir_sn for objects from previous levels
                            update_parent_dir_sn(previous_depth_objects , curr_depth_objects , global_current_depth ,
                                                 i , mem_pool , ht_files , ht_dirs , roots);
                            //Update Object lists
                            listDestroy(previous_depth_objects); //Empty the previous_depth_objects list
                            previous_depth_objects = listCopy(curr_depth_objects);//Copy the curr_depth_objects list to the previous_depth_objects
                            listClear(curr_depth_objects); //Empty the curr_depth_objects list
                            global_current_depth = depth;
                        }
                        break;
                    case 5: /* FILE SIZE */
                        file_size = case_5_file_size(buff);
                        break;
                    case 6: /* FILE ATTRIBUTES VALUE */
                        obj_type = case_6_file_attribute(buff);
                        if( ( file_size == 0 ) && ( obj_type == 'F' ) ){ // ignore zero size files
                            is_zero_size_file = true;
                        }
                        break;
                    case 7: /* FILE ID */
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
                                roots[i] = ht_set(ht_dirs , root_id , -1 , dir_sn ,DIR_SIZE , 'D' , &object_exists_in_hash_already , 0 , dedup_type , mem_pool);
                                dir_sn++;
                                free(root_id);
                                set_root = false;
                            }
                            //Create Directory Object with the retrieved data
                            ht_set(ht_dirs, object_id, depth, dir_sn, DIR_SIZE , 'D' , &object_exists_in_hash_already , 0, dedup_type , mem_pool);
                            dir_sn++;
                        }
                        break;
                    case 13: /* Line 13 is SV */
                        case_13_VS(input_file , buff , &block_line_count ,
                                   &read_empty_line_chucnks , depth , object_id,file_size,
                                   &file_was_created, &finished_process_blocks , mem_pool,dedup_type,
                                   ht_files , ht_blocks, ht_physical_files,
                                   &files_sn , &physical_files_sn, &blocks_sn);
                        // Add object (File or Directory) to curr_depth_objects list
                        if ((obj_type == 'F') && (is_zero_size_file == false) && (file_was_created == true)){
                            Object_Info oi_file = object_info_create(object_id , (files_sn - 1) , parent_dir_id , 'F');
                            listInsertLast(curr_depth_objects , oi_file);
                            object_info_destroy(oi_file); //The list adds a copy of this object and it is no longer needed
                        } else if(obj_type == 'D'){ //Adding Directory Object to HashTable
                            //add directory to curr_depth_objects list in order to later find the parent directory
                            Object_Info oi_dir = object_info_create(object_id , (dir_sn - 1) , parent_dir_id , 'D');
                            listInsertLast(curr_depth_objects , oi_dir);
                            object_info_destroy(oi_dir); //The list adds a copy of this object and it is no longer needed
                        }
                        break;
                    default:
                        break;
                }

                if(read_empty_line_chucnks == false && finished_process_blocks == false){
                    fgets(buff, BUFFER_SIZE , input_file); //read next line in current block
                    clear_line(buff);
                    block_line_count++;
                }
            } /* Processing Object */

            /***********************************************************************************************/
            /******************* WE HAVE REACHED THE END OF THE CURRENT INPUT OBJECT !!! ********************/
            if(!feof(input_file)){ //Update parameters that are relevant to a single object
                block_line_count = 0; /* Zero the line count for the next block */
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

        fprintf(monitor_file, "(Parser) --> --- Finished reading the input file ---\n");
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        fprintf(monitor_file, "(Parser)-->              %s             \n" , asctime (timeinfo));
        fflush(monitor_file);
        //This means we have reached a new depth and can update parent_dir_sn for objects from previous levels
        update_parent_dir_sn(previous_depth_objects , curr_depth_objects , global_current_depth ,
                             i , mem_pool , ht_files , ht_dirs , roots);
        if(finished_reading_file == true){
            global_current_depth = 0;
            listClear(curr_depth_objects); //Empty the curr_depth_objects list
            listClear(previous_depth_objects); //Empty the curr_depth_objects list
            block_line_count = 0; /* Zero the line count for the next block */
            read_empty_line_chucnks = false;
            is_zero_size_file = false;
            file_was_created = false;
            finished_process_blocks = false;
            object_exists_in_hash_already = false;
            finished_reading_file = false;
        }
    } /* (1) Read an Input File */
    printf("Creating CSV .... \n");
    fprintf(monitor_file, "(Parser) --> --- Creating CSV .... ---\n");
    fflush(monitor_file);
    print_ht_to_CSV(dedup_type, files_to_read , num_input_files,
                    blocks_sn, files_sn, dir_sn, physical_files_sn,
                    ht_files, ht_blocks, ht_dirs, ht_physical_files);
    printf("Freeing up Memory .... \n");
    fprintf(monitor_file, "(Parser) --> --- Freeing up Memory ....  ---\n");
    fflush(monitor_file);
    //Free All Hash tables and Lists
    listDestroy(previous_depth_objects);
    listDestroy(curr_depth_objects);
    fclose(monitor_file);
    memory_pool_destroy(mem_pool);
    free(mem_pool);
    return 0;
}
