/* This file contains a rough implementation of an L1 cache in the absence of an L2 cache*/
#include <stdlib.h>
#include <stdio.h>

struct cache_blk_t
{ // note that no actual data will be stored in the cache
  unsigned long tag;
  char valid;
  char dirty;
  unsigned LRU; //to be used to build the LRU stack for the blocks in a cache set
};

struct cache_t
{
  // The cache is represented by a 2-D array of blocks.
  // The first dimension of the 2D array is "nsets" which is the number of sets (entries)
  // The second dimension is "assoc", which is the number of blocks in each set.
  int nsets;                   // number of sets
  int blocksize;               // block size
  int assoc;                   // associativity
  int mem_latency;             // the miss penalty
  struct cache_blk_t **blocks; // a pointer to the array of cache blocks
};

struct cache_t *cache_create(int size, int blocksize, int assoc, int mem_latency)
{
  int i, nblocks, nsets;
  struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

  nblocks = size * 1024 / blocksize; // number of blocks in the cache
  nsets = nblocks / assoc;           // number of sets (entries) in the cache
  C->blocksize = blocksize;
  C->nsets = nsets;
  C->assoc = assoc;
  C->mem_latency = mem_latency;

  C->blocks = (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

  for (i = 0; i < nsets; i++)
  {
    C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
  }
  return C;
}
//------------------------------

int updateLRU(struct cache_t *cp, int index, int way)
{
  int k;
  for (k = 0; k < cp->assoc; k++)
  {
    if (cp->blocks[index][k].LRU < cp->blocks[index][way].LRU)
      cp->blocks[index][k].LRU = cp->blocks[index][k].LRU + 1;
  }
  cp->blocks[index][way].LRU = 0;
}

int cache_access(struct cache_t **cp, int cache_index, unsigned long address, int access_type, int latency)
{
  int i;
  int block_address;
  int index;
  int tag;
  int way;
  int max;


if (cp[index] == NULL) 
{
	return latency + M_miss_penalty;
}

  latency += cp[cache_index]->mem_latency;
  block_address = (address / cp[cache_index]->blocksize);
  tag = block_address / cp[cache_index]->nsets;
  index = block_address - (tag * cp[cache_index]->nsets);

  /* look for the requested block */
  for (i = 0; i < cp[cache_index]->assoc; i++)
  { 
    if (cp[cache_index]->blocks[index][i].tag == tag && cp[cache_index]->blocks[index][i].valid == 1)
    {
      updateLRU(cp[cache_index], index, i);
      if (access_type == 1)
        cp[cache_index]->blocks[index][i].dirty = 1;	// if writing, update dirty bit
      return (latency); /* a cache hit */
    }
  }

  /*data was not found*/

  /*find a block to write to*/
  /*check for empty blocks*/
  for (way = 0; way < cp[cache_index]->assoc; way++) /* look for an invalid entry */
    if (cp[cache_index]->blocks[index][way].valid == 0)
    {
      cp[cache_index]->blocks[index][way].valid = 1;	// valid memory
      cp[cache_index]->blocks[index][way].tag = tag;	// update tag
      updateLRU(cp[cache_index], index, way);			// update LRU
      cp[cache_index]->blocks[index][way].dirty = 0;	// reset dirty bit
      if (access_type == 1)
        latency += M_miss_penalty;	// if write miss, update memory
      return latency; /* an invalid entry is available*/
    }
  /*Couldn't find an empty block*/

  /*find least recently used block and evict*/
  
  /*gets the least recently used block*/
  max = cp[cache_index]->blocks[index][0].LRU; /* find the LRU block */
  way = 0;

  for (i = 1; i < cp[cache_index]->assoc; i++)			// look through cache for LRU block
    if (cp[cache_index]->blocks[index][i].LRU > max)
    {
      max = cp[cache_index]->blocks[index][i].LRU;		// update indices of LRU
      way = i;
    }
  /*end of gets the least recently used block*/

  // if evicting from L2, must search both L1s -> don't evict if block is in either
  if (cp[cache_index]){}

  /*end of check*/
  
  /*we can evict this block*/
  if (cp[cache_index]->blocks[index][way].dirty == 1)	{  // Write back into memory if dirty bit == 1
    int evicted_address = index + cp[cache_index]->blocks[index][way].tag * cp[cache_index]->nsets;
    latency += cache_write_back(cp, cache_index++, evicted_address, access_type);
  }     

  /*go down and "find" the data that we need to write to the block we just cleared*/
  latency = cache_access(cp, cache_index++, address, access_type, latency);

  /*write to the block we cleared out*/
  // add memory latency for writing new block
  cp[cache_index]->blocks[index][way].tag = tag;		    // update tag
  updateLRU(cp[cache_index], index, way);			        	// update LRU
  cp[cache_index]->blocks[index][i].dirty = 0;			    // reset dirty bit

  return cache_access(cp, cache_index++, address, access_type, latency);
}

int cache_write_back(struct cache_t **cp, int cache_index, unsigned long evicted_address)
{
  if (cp[cache_index] == NULL){
    return M_miss_penalty;
  }

  int block_address = (evicted_address / cp[cache_index]->blocksize);
  int tag = block_address / cp[cache_index]->nsets;
  int index = block_address - (tag * cp[cache_index]->nsets);  

  for (int i = 0; i < cp[cache_index]->assoc; i++)
  { 
    if (cp[cache_index]->blocks[index][i].tag == tag && cp[cache_index]->blocks[index][i].valid == 1)
    {
      updateLRU(cp[cache_index], index, i);
      
      // write back to the block
      cp[cache_index]->blocks[index][i].dirty = 1;	
    }
  }
  return cp[cache_index]->mem_latency;
}