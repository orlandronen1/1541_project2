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

  nblocks = (size * 1024) / blocksize; // number of blocks in the cache
  //nblocks = (size) / blocksize;
  nsets = nblocks / assoc;           // number of sets (entries) in the cache
  C->blocksize = blocksize;
  C->nsets = nsets;
  C->assoc = assoc;
  //printf("%d, ", assoc);
  C->mem_latency = mem_latency;

  C->blocks = (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t *));

  for (i = 0; i < nsets; i++)
  {
    C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
  }
  return C;
}
//------------------------------

void print_slot(struct cache_blk_t *blocks, int assoc){
  for (int i = 0; i < assoc; i++){
    //printf("(%d, %lu)", blocks[i].valid, blocks[i].tag);
  }
  //printf("\n");
}

void print_slot_lru(struct cache_blk_t *blocks, int assoc){
  for (int i = 0; i < assoc; i++){
    //printf("(%d, %d)", blocks[i].valid, blocks[i].LRU);
  }
  //printf("\n");
}

int updateLRU(struct cache_t *cp, int index, int way)
{
  //printf("update: ");
  int k;
  for (k = 0; k < cp->assoc; k++)
  {
    cp->blocks[index][k].LRU = cp->blocks[index][k].LRU + 1;
  }
  cp->blocks[index][way].LRU = 0;
  //print_slot_lru( cp->blocks[index], cp->assoc);
}

int cache_write_back(struct cache_t **cp, int cache_index, unsigned long evicted_address);



int cache_access(struct cache_t **cp, int cache_index, unsigned long address, int access_type, int latency)
{

  int i;
  int block_address;
  int index;
  int tag;
  int way;
  int max;

  //printf("cache %d, %d; addr: %lu\n", cache_index, access_type, address);

if (cp[cache_index] == NULL) 
{
  //printf("mem\n\n");
	return latency + M_miss_penalty;
} else if (cache_index  == 1){
  S_accesses++;
}


  latency += cp[cache_index]->mem_latency;
  block_address = (address / cp[cache_index]->blocksize);
  tag = block_address / cp[cache_index]->nsets;
  index = block_address - (tag * cp[cache_index]->nsets);

  //printf("index: %d, tag: %d\n", index, tag);
  //printf("access: %d\n", access_type);
  print_slot(cp[cache_index]->blocks[index], cp[cache_index]->assoc);

  /* look for the requested block */
  for (i = 0; i < cp[cache_index]->assoc; i++)
  { 
    ////printf("%d, %d\n", cp[cache_index]->blocks[index][i].tag == tag, cp[cache_index]->blocks[index][i].valid == 1);
    if (cp[cache_index]->blocks[index][i].tag == tag && cp[cache_index]->blocks[index][i].valid == 1)
    {
      //printf("hit");
      updateLRU(cp[cache_index], index, i);
      ////printf("dirty: %d", cp[cache_index]->blocks[index][i].dirty);
      if (access_type == 1)
      {
        printf("Write hit");
        cp[cache_index]->blocks[index][i].dirty = 1;	// if writing, update dirty bit
      }
        
      return (latency); /* a cache hit */
    }
  }

  /*data was not found*/
  if (cache_index == 1){
    S_misses++;
  }
  /*find a block to write to*/
  /*check for empty blocks*/
  for (way = 0; way < cp[cache_index]->assoc; way++) /* look for an invalid entry */
    if (cp[cache_index]->blocks[index][way].valid == 0)
    {
      //printf("empty block found\n");
      cp[cache_index]->blocks[index][way].valid = 1;	// valid memory
      cp[cache_index]->blocks[index][way].tag = tag;	// update tag
      updateLRU(cp[cache_index], index, way);			// update LRU
      cp[cache_index]->blocks[index][way].dirty = 0;	// reset dirty bit
       
      goto get_data;     
    }
  /*Couldn't find an empty block*/

  /*find least recently used block and evict*/
  
  /*gets the least recently used block*/
  //printf("evict");
  struct cache_blk_t *copy = (struct cache_blk_t *)calloc(cp[cache_index]->assoc, sizeof(struct cache_blk_t));

  for (int j = 0; j < cp[cache_index]->assoc; j++)
  {
    copy[j].tag = cp[cache_index]->blocks[index][j].tag;
    copy[j].valid = cp[cache_index]->blocks[index][j].valid;
    copy[j].dirty = cp[cache_index]->blocks[index][j].dirty;
    copy[j].LRU = cp[cache_index]->blocks[index][j].LRU;
  }


  int compare_function(const void* p1, const void* p2) 
  {
    return ( ((struct cache_blk_t*)p2)->LRU - ((struct cache_blk_t*)p1)->LRU);
  }
  qsort(copy, cp[cache_index]->assoc, sizeof(struct cache_blk_t), compare_function);
  
  print_slot(copy, cp[cache_index]->assoc);
  // check both L1's to see if LRU is in either. if it is, go to next LRU
  int found = 1;
  int fi,fblock_address, fvalid, ftag, findex;
  for (fi = 0; fi < cp[cache_index]->assoc && found == 1; fi++)
  {
    if (cache_index == 0)
      break;
    found = 0;

    fblock_address = index + copy[fi].tag * cp[cache_index]->nsets;
    //printf("evicting? %lu", copy[fi].tag);
    ftag = fblock_address / cp[0]->nsets;

    findex = fblock_address - (ftag * cp[0]->nsets);
    print_slot(cp[0]->blocks[findex], cp[0]->assoc);
    for (int fj = 0; fj < cp[0]->assoc; fj++)
    {
      
      if (cp[0]->blocks[findex][fj].tag == ftag && cp[0]->blocks[findex][fj].valid == 1)  
      {
        found = 1;
      }
    }
    //printf("no\n");
    //search Dcache
    
    ftag = fblock_address / cp[3]->nsets;
    findex = fblock_address - (ftag * cp[3]->nsets);
    //printf(cp[3]->blocks[findex], cp[0]->assoc);

    for (int fj = 0; fj < cp[3]->assoc; fj++)
    {
      //printf("other: %d, %lu, %d", cp[3]->blocks[findex][fj].valid , cp[3]->blocks[findex][fj].tag, ftag);
      if (cp[3]->blocks[findex][fj].tag == ftag && cp[3]->blocks[findex][fj].valid == 1) 
      {
        found = 1;

      }
    }
                //printf("instr no\n");

    ftag = fblock_address / cp[cache_index]->nsets;
    
    //if block is found in either, set found to 1
    //if found is still 0 after checking both, for loop ends and can use the block in copy[fi] -> have to find it in the cache again, though
  }


  //printf("ftag: %d", ftag);
  for (way = 0; way < cp[cache_index]->assoc; way++)
  { 
    if (cp[cache_index]->blocks[index][way].tag == ftag)
    {
      break;
    }
  }

  // after done w/ copy, 
  free(copy);
  
  /*end of check*/
  
  /*we can evict this block*/
  if (cp[cache_index]->blocks[index][way].dirty == 1)	{  // Write back into memory if dirty bit == 1
    int evicted_address = index + cp[cache_index]->blocks[index][way].tag * cp[cache_index]->nsets;
    latency += cache_write_back(cp, cache_index + 1, evicted_address);
  }     

  get_data:

  /*go down and "find" the data that we need to write to the block we just cleared*/
  //printf("Next level\n");
  latency = cache_access(cp, cache_index + 1, address, access_type, latency);

  //printf("index: %d, way: %d, tag:  %d", index, way, tag);
  /*write to the block we cleared out*/
  // add memory latency for writing new block
  cp[cache_index]->blocks[index][way].tag = tag;		    // update tag
  updateLRU(cp[cache_index], index, way);			        	// update LRU
  cp[cache_index]->blocks[index][way].dirty = 0;			    // reset dirty bit

  //printf(cp[cache_index]->blocks[index], cp[cache_index]->assoc);


  return latency;
}

int cache_write_back(struct cache_t **cp, int cache_index, unsigned long evicted_address)
{
  //printf("wrtie back\n");
  if (cp[cache_index] == NULL){
    return M_miss_penalty;
  }else if (cache_index == 1){
    S_accesses++;
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