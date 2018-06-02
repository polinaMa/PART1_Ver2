//
// Created by Polina on 04-May-18.
//
#ifndef DEDUPLICATIONPROJECT_MEMORY_POOL_H
#define DEDUPLICATIONPROJECT_MEMORY_POOL_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


#define POOL_INITIAL_SIZE 2048*8192

typedef struct memory_pool_t
{
	uint32_t next_free_index; //index inside the current pool
	uint32_t next_free_pool_index; //index inside next pool that is free
	uint32_t arr[POOL_INITIAL_SIZE]; // array
	struct memory_pool_t* next_pool; //pointer to the next allocated pool
	struct memory_pool_t* next_free_pool_ptr; //pointer to the next allocated pool
}Memory_pool, *PMemory_pool;

/*
	@Function:	memory_pool_init
	@Params:	pool -	Pointer to the pool struct to initialize.
	@Desc:		Pool struct will be initialized, allocated and the memory would be set to 0.
*/
void* memory_pool_init(PMemory_pool pool);


/*
	@Function:	memory_pool_alloc
	@Params:	pool -	Pointer to the pool from which to allocate memory from.
				size -	The size (in bytes) of memory to allocate.
	@Desc:		Memory of the requested size will be allocated from the pool,
                if required the pool would be extended.
				A pointer to the beginning of the memory would be place in the res pointer.
*/
void* memory_pool_alloc(PMemory_pool pool, uint32_t size);

/*
	@Function:	memory_pool_destroy
	@Params:	pool -	Pointer to the pool to destroy
	@Desc:		All memory of the pool will be freed.
*/
void memory_pool_destroy(PMemory_pool pool);

#endif //DEDUPLICATIONPROJECT_MEMORY_POOL_H
