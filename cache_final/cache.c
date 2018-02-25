/*
 * cache.c
 */


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "cache.h"
#include "main.h"

/* cache configuration parameters */
static int cache_split = 0;
static int cache_usize = DEFAULT_CACHE_SIZE;
static int cache_isize = DEFAULT_CACHE_SIZE;
static int cache_dsize = DEFAULT_CACHE_SIZE;
static int cache_block_size = DEFAULT_CACHE_BLOCK_SIZE;
static int words_per_block = DEFAULT_CACHE_BLOCK_SIZE / WORD_SIZE;
static int cache_assoc = DEFAULT_CACHE_ASSOC;
static int cache_writeback = DEFAULT_CACHE_WRITEBACK;
static int cache_writealloc = DEFAULT_CACHE_WRITEALLOC;

/* cache model data structures */
static Pcache icache;
static Pcache dcache;
static Pcache ucache; // this is the pointer for the unifed cache
static cache c1;
static cache c2;
static cache_stat cache_stat_inst;
static cache_stat cache_stat_data;

/************************************************************/
void set_cache_param(param, value)
  int param;
  int value;
{

  switch (param) {
  case CACHE_PARAM_BLOCK_SIZE:
    cache_block_size = value;
    words_per_block = value / WORD_SIZE;
    break;
  case CACHE_PARAM_USIZE:
    cache_split = FALSE;
    cache_usize = value;
    break;
  case CACHE_PARAM_ISIZE:
    cache_split = TRUE;
    cache_isize = value;
    break;
  case CACHE_PARAM_DSIZE:
    cache_split = TRUE;
    cache_dsize = value;
    break;
  case CACHE_PARAM_ASSOC:
    cache_assoc = value;
    break;
  case CACHE_PARAM_WRITEBACK:
    cache_writeback = TRUE;
    break;
  case CACHE_PARAM_WRITETHROUGH:
    cache_writeback = FALSE;
    break;
  case CACHE_PARAM_WRITEALLOC:
    cache_writealloc = TRUE;
    break;
  case CACHE_PARAM_NOWRITEALLOC:
    cache_writealloc = FALSE;
    break;
  default:
    printf("error set_cache_param: bad parameter value\n");
    exit(-1);
  }

}
/************************************************************/

/************************************************************/

// This function will create two cache structures if split is true o.w just creates one cache structure
// The deafault cache structure is c1. In the case of a split c1-> instructions and c2-> data.
void init_cache()
{
  switch (cache_split) {

    case TRUE:  // TRUE is for split cache
    printf(" \n Split : %d \n", cache_split);
      icache = &c1;
      dcache = &c2;
      init_cache_st();
      break;

    case FALSE : // FALSE is for unified cache
    printf(" \n No Split :%d \n", cache_split);
      ucache = icache = dcache = &c1;
      init_cache_st_unified();
      break;
  }



}
/************************************************************/
/************************************************************/

/*
  In case of a split both caches have the same structure, size and memebers,
  then the definition of both is the same.
*/
void init_cache_st()
{
  /* Defining the size of the cache */
  icache -> size = cache_isize;
  dcache -> size = cache_dsize;

  /* This defines the assoaciativity k-way of the cache k=1 -> Direct */
  icache -> associativity = cache_assoc;
  dcache -> associativity = cache_assoc;

  /* Calculating the number of sets */
  icache -> n_sets = c1.size/(cache_assoc*cache_block_size);
  dcache -> n_sets = c2.size/(cache_assoc*cache_block_size);

  /* The offset, index and tag sizes are calculated.*/
  icache -> offset_s = LOG2(words_per_block*4); // offset bits
  icache -> index_s = LOG2((*icache).n_sets); // index bits
  icache -> tag_s = 32 - (*icache).offset_s - (*icache).index_s; // tag bits

  dcache -> offset_s = LOG2(words_per_block*4); // offset bits
  dcache -> index_s = LOG2((*dcache).n_sets); // index bits
  dcache -> tag_s = 32 - (*dcache).offset_s - (*dcache).index_s; // tag bits

  /*  Calculating the masks */
  icache -> offset_mask = 0xffffffff >> (32- ((*icache).offset_s) ); // offset mask
  icache -> index_mask = 0xffffffff >> (32 - ((*icache).index_s)); 	// calculates index mask, but ignores the offset
  icache -> index_mask = icache -> index_mask << ((*icache).offset_s ); // index mask caring about the offset
  icache -> tag_mask  = 0xffffffff << ((*icache).offset_s  + (*icache).index_s ) ; // tag mask

  dcache -> offset_mask = 0xffffffff >> (32- ((*dcache).offset_s) ); // offset mask
  dcache -> index_mask = 0xffffffff >> (32 - ((*dcache).index_s)); 	// calculates index mask, but ignores the offset
  dcache -> index_mask = dcache -> index_mask << ((*dcache).offset_s ); // index mask caring about the offset
  dcache -> tag_mask  = 0xffffffff << ((*dcache).offset_s  + (*dcache).index_s ) ;  // tag mask

  /* Defining LRU_head and tail */

  icache -> LRU_head = (Pcache_line *)malloc(sizeof(Pcache_line)*icache->n_sets);
  dcache -> LRU_head = (Pcache_line *)malloc(sizeof(Pcache_line)*dcache->n_sets);

  icache -> LRU_tail = (Pcache_line *)malloc(sizeof(Pcache_line)*icache->n_sets);
  dcache -> LRU_tail = (Pcache_line *)malloc(sizeof(Pcache_line)*dcache->n_sets);

  /*
  set_contents := is a int* to a a vector keeps track of how many cache_lines that are in each set
  It is dynamic and will change.
  */
  icache -> set_contents = (int *)malloc(sizeof(int)*icache->n_sets);
  dcache -> set_contents = (int *)malloc(sizeof(int)*dcache->n_sets);

  /* This makes NULL the default for cache_line pointers, and 0 for the number of cache lines in the sets */

  for( int a ; a < icache -> n_sets; a++)
  {
    icache -> LRU_head[a] = NULL ;
    dcache -> LRU_head[a] = NULL ;

    icache -> LRU_tail[a] = NULL ;
    dcache -> LRU_tail[a] = NULL ;

    icache -> set_contents[a] = 0 ;
    dcache -> set_contents[a] = 0 ;
  }

  /* contents */

  /* Cache statistics to zero*/
  cache_stat_inst.accesses        = 0 ;
  cache_stat_inst.misses          = 0 ;
  cache_stat_inst.replacements    = 0 ;
  cache_stat_inst.demand_fetches  = 0 ;
  cache_stat_inst.copies_back     = 0 ;


  cache_stat_data.accesses        = 0 ;
  cache_stat_data.misses          = 0 ;
  cache_stat_data.replacements    = 0 ;
  cache_stat_data.demand_fetches  = 0 ;
  cache_stat_data.copies_back     = 0 ;

// Prints the masks
//       int bits_o[32];
//         int bits_i[32];
//          int bits_t[32];
//
//
//              for (int i = 0; i < 32; i = i+1)
//              {
//                  bits_o[i] = (c1.offset_mask >> i) & 1;
//                  bits_i[i] = (c1.index_mask >> i) & 1;
//                  bits_t[i] = (c1.tag_mask >> i) & 1;
//
//
//              }
//          	  printf("\n");
//
//          printf("off:");
//         	    for (         int bits_t[32];
//
// int i = 31; i >-1; i = i-1) {
//                 printf("%d",bits_o[i] );
//             }
//         	printf("\n");
//
//
//
//             printf("ind:");
//             	    for (int i = 31; i >-1; i = i-1) {
//                 printf("%d",bits_i[i] );
//             }
//         	printf("\n");
//
//
//
//             printf("tag:");
//             	    for (int i = 31; i >-1; i = i-1) {
//                 printf("%d",bits_t[i] );
//             }
//         	printf("\n");



}

/******
******************************************************/

/************************************************************/
/************************************************************/

/*
  In case of a split both caches have the same structure, size and memebers,
  then the definition of both is the same.
*/
void init_cache_st_unified()
{
  /* Defining the size of the cache */
  ucache -> size = cache_usize;

  /* This defines the assoaciativity k-way of the cache k=1 -> Direct */
  ucache -> associativity = cache_assoc;

  /* Calculating the number of sets */
  ucache -> n_sets = c1.size/(cache_assoc*cache_block_size);

  /* The offset, index and tag sizes are calculated.*/
  ucache -> offset_s = LOG2(words_per_block*4); // offset bits
  ucache -> index_s = LOG2((*ucache).n_sets); // index bits
  ucache -> tag_s = 32 - (*ucache).offset_s - (*ucache).index_s; // tag bits

  /*  Calculating the masks */
  ucache -> offset_mask = 0xffffffff >> (32- ((*ucache).offset_s) ); // offset mask
  ucache -> index_mask = 0xffffffff >> (32 - ((*ucache).index_s)); 	// calculates index mask, but ignores the offset
  ucache -> index_mask = ucache -> index_mask << ((*ucache).offset_s ); // index mask caring about the offset
  ucache -> tag_mask  = 0xffffffff << ((*ucache).offset_s  + (*ucache).index_s ) ; // tag mask

  /* Defining LRU_head and tail */

  ucache -> LRU_head = (Pcache_line *)malloc(sizeof(Pcache_line)*ucache->n_sets);

  ucache -> LRU_tail = (Pcache_line *)malloc(sizeof(Pcache_line)*ucache->n_sets);

  /*
  set_contents := is a int* to a a vector keeps track of how many cache_lines that are in each set
  It is dynamic and will change.
  */
  ucache -> set_contents = (int *)malloc(sizeof(int)*ucache->n_sets);

  /* This makes NULL the default for cache_line pointers, and 0 for the number of cache lines in the sets */

  for( int a ; a < ucache -> n_sets; a++)
  {
    ucache -> LRU_head[a] = NULL ;

    ucache -> LRU_tail[a] = NULL ;

    ucache -> set_contents[a] = 0 ;
  }

  /* contents */

  /* Cache statistics to zero*/
  cache_stat_inst.accesses        = 0 ;
  cache_stat_inst.misses          = 0 ;
  cache_stat_inst.replacements    = 0 ;
  cache_stat_inst.demand_fetches  = 0 ;
  cache_stat_inst.copies_back     = 0 ;


  cache_stat_data.accesses        = 0 ;
  cache_stat_data.misses          = 0 ;
  cache_stat_data.replacements    = 0 ;
  cache_stat_data.demand_fetches  = 0 ;
  cache_stat_data.copies_back     = 0 ;


}



/************************************************************/

/* This function detects the type of acces and invokes the correct function*/
void perform_access(addr, access_type)
  unsigned addr, access_type;
{
  // Check if the cache is split
  if(cache_split){
    switch(access_type){

		    case 0:
		      perform_access_d_load(addr, dcache);
		      break;
		    case 1:
		      perform_access_d_write(addr, dcache);
		      break;
		    case 2:
		      perform_access_i(addr, icache);
		      break;
	  }
  } else{
    switch(access_type){
		    case 0:
		     perform_access_d_load(addr, ucache);
		      break;
		    case 1:
		     perform_access_d_write(addr, ucache);
		      break;
		    case 2:
		      perform_access_i(addr, ucache);
		      break;

        }
      }

}
/************************************************************/
/************************************************************/



/* Remember c1 is the the memory for instructions, and the default in other cases*/
void perform_access_i(unsigned addr, Pcache c)
{ cache_stat_inst.accesses ++; // Acess instruction cache
  // Jump = set


  unsigned jump  = (addr & (c->index_mask) ) >> (c->offset_s);
  unsigned t_tag = (addr & (c->tag_mask) ) >> (c->index_s + c->offset_s);
  if(c->n_sets == 1){ jump =0;}

  if(c->set_contents[jump] == 0){ //Checks if there is a block already in the set
    // c->LRU_head[jump] = malloc(sizeof(cache_line));
    cache_stat_inst.misses ++; // there is a miss
    cache_stat_inst.demand_fetches += words_per_block; //load the block (number of words)
    // c->LRU_head[jump]->tag = t_tag;
    c->set_contents[jump] ++;
    Pcache_line c_line = malloc(sizeof(cache_line));
    c_line -> tag      = t_tag;

    insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]), c_line);

  } else{ // The set is not empty, thus it has to search among the existing blocks in the set
    // Search the cache line
    Pcache_line hit;
    search_line(t_tag, c, jump, &hit);

      if( hit != NULL){ // HIT
        delete(&(c->LRU_head[jump]), &(c->LRU_tail[jump]),hit);
        insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]),hit);
      }else{ // Miss
        // CREATES A NEW CACHE BLOCK TO BE INSERTED
        Pcache_line c_line = malloc(sizeof(cache_line));
        c_line -> tag      = t_tag;
        cache_stat_inst.misses ++; //there is a miss
        cache_stat_inst.demand_fetches += words_per_block; //load the block (number of words)

        if(c->set_contents[jump] < c->associativity){ // There is space to load a block without deleting
          // c->LRU_head[jump] = malloc(sizeof(cache_line));
          // c->LRU_head[jump]->tag = t_tag;
          c->set_contents[jump] ++;
          insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]), c_line);
        }else{ // set full, delete tail, insert as head, REPLACE TAKES PLACE  --------REPLACE!
          // for WRITE_BACK
          cache_stat_inst.replacements ++; //load the block (number of words)

            if( cache_writeback && cache_split == 0){
                cache_stat_data.copies_back += words_per_block *(c->LRU_tail[jump]->dirty); //load the block (number of words)
            }

          delete(&(c->LRU_head[jump]),&(c->LRU_tail[jump]),c->LRU_tail[jump]);
          insert(&(c->LRU_head[jump]),&(c->LRU_tail[jump]),c_line);

        }

    } // end else MISS
  }

}
/************************************************************/
/************************************************************/
void perform_access_d_load(unsigned addr, Pcache c)
{ cache_stat_data.accesses ++; // Acess instruction cache
  // Jump = set

  unsigned jump  = (addr & (c->index_mask) ) >> (c->offset_s);
  unsigned t_tag = (addr & (c->tag_mask) ) >> (c->index_s + c->offset_s);
  if(c->n_sets == 1){ jump =0;}


  if(c->set_contents[jump] == 0){ //Checks if there is a block already in the set
    // c->LRU_head[jump] = malloc(sizeof(cache_line));
    cache_stat_data.misses ++; // there is a miss
    cache_stat_data.demand_fetches += words_per_block; //load the block (number of words)
    // c->LRU_head[jump]->tag = t_tag;
    c->set_contents[jump] ++;
    Pcache_line c_line = malloc(sizeof(cache_line));
    c_line -> tag      = t_tag;

    insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]), c_line);

  } else{ // The set is not empty, thus it has to search among the existing blocks in the set
    // Search the cache line
    Pcache_line hit;
    search_line(t_tag, c, jump, &hit);

      if( hit != NULL){ // HIT
        delete(&(c->LRU_head[jump]), &(c->LRU_tail[jump]),hit);
        insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]),hit);
      }else{ // Miss
        // CREATES A NEW CACHE BLOCK TO BE INSERTED
        Pcache_line c_line = malloc(sizeof(cache_line));
        c_line -> tag      = t_tag;
        cache_stat_data.misses ++; //there is a miss
        cache_stat_data.demand_fetches += words_per_block; //load the block (number of words)

        if(c->set_contents[jump] < c->associativity){ // There is space to load a block without deleting
          // c->LRU_head[jump] = malloc(sizeof(cache_line));
          // c->LRU_head[jump]->tag = t_tag;
          c->set_contents[jump] ++;
          insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]), c_line);
        }else{ // set full, delete tail, insert as head, REPLACE TAKES PLACE  --------REPLACE!
          // for WRITE_BACK
          cache_stat_data.replacements ++; //load the block (number of words)

              if( cache_writeback){
                cache_stat_data.copies_back += words_per_block *(c->LRU_tail[jump]->dirty); //load the block (number of words)
              }

          delete(&(c->LRU_head[jump]),&(c->LRU_tail[jump]),c->LRU_tail[jump]);
          insert(&(c->LRU_head[jump]),&(c->LRU_tail[jump]),c_line);

        }

    }
  }

}


/************************************************************/
/************************************************************/

void perform_access_d_write(unsigned addr, Pcache c)
{ cache_stat_data.accesses ++; // Acess instruction cache
  // Jump = set


  unsigned jump  = (addr & (c->index_mask) ) >> (c->offset_s);
  unsigned t_tag = (addr & (c->tag_mask) ) >> (c->index_s + c->offset_s);
  if(c->n_sets == 1){ jump =0;}

  //printf("\n Jump: %u \n", jump);




  // SInce it is write date, we also need to check for write_allocate policy
  if(c->set_contents[jump] == 0 && cache_writealloc){ //Checks if there is a block already in the set
    // c->LRU_head[jump] = malloc(sizeof(cache_line));

    // c->LRU_head[jump]->tag = t_tag;
    c->set_contents[jump] ++;
    Pcache_line c_line = malloc(sizeof(cache_line));
    c_line -> tag      = t_tag;

    cache_stat_data.misses ++; // there is a miss
    cache_stat_data.demand_fetches += words_per_block; //load the block (number of words)

    if(cache_writeback){ // CHECK WRITE_BACK
      c_line->dirty   = 1; //dirty bit set to zero
    }else{cache_stat_data.copies_back ++;}

    insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]), c_line);

  } else if(c->set_contents[jump] == 0 && cache_writealloc != 1){ // nosthing to search NO-ALLOCATE

      // cache_stat_data.demand_fetches ++; // depends fo statistics
      cache_stat_data.misses ++;
      cache_stat_data.copies_back ++;

  } else{ // There is somenthing loaded in the set, then we need to search

      Pcache_line hit;
      search_line(t_tag, c, jump, &hit);

      if( hit != NULL){ // HIT
        if(cache_writeback){ // Hit ->CHECK WRITE_BACK
          hit->dirty   = 1; //dirty bit set to one
        }else{ // Hit -> no_write_back
          cache_stat_data.copies_back ++;
        }

        delete(&(c->LRU_head[jump]), &(c->LRU_tail[jump]),hit);
        insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]),hit);

      } else if(cache_writealloc){ // MISS & WRITE_ALLOCATE

        cache_stat_data.misses ++;
        cache_stat_data.demand_fetches += words_per_block; // depends fo statistics *************************************



        Pcache_line c_line = malloc(sizeof(cache_line));
        c_line -> tag      = t_tag;

        if(cache_writeback){ // CHECK WRITE_BACK
          c_line->dirty   = 1; //dirty bit set to zero
        }else{cache_stat_data.copies_back ++;}

        if(c->set_contents[jump] < c->associativity){ // There is space to load a block without deleting
          c->set_contents[jump] ++;
          insert(&(c->LRU_head[jump]), &(c->LRU_tail[jump]), c_line);
        }else{ // REPLACE & FULL SET

          cache_stat_data.replacements ++; // depends fo statistics

              if(cache_writeback){
                cache_stat_data.copies_back += words_per_block *(c->LRU_tail[jump]->dirty) ;
              }

          delete(&(c->LRU_head[jump]),&(c->LRU_tail[jump]),c->LRU_tail[jump]);
          insert(&(c->LRU_head[jump]),&(c->LRU_tail[jump]),c_line);

        }


      }else{ // MISS & NOOOO-WRITE_ALLOCATE

          // cache_stat_data.demand_fetches += words_per_block;
          // cache_stat_data.demand_fetches ++;
          cache_stat_data.copies_back ++;
          cache_stat_data.misses ++;



      }



  } // ends search else


}


/************************************************************/
/************************************************************/
/* Returns the argument Pcache_line head.
  If it found the tag, it modifies the address of the cache line with the tag
  If it didnt find the tag, it modifies head=NULL as address.
*/
void search_line(unsigned t_tag, Pcache c, unsigned jump, Pcache_line *aux){
  int find =1;
  Pcache_line head = c->LRU_head[jump];
  while(find<=c->set_contents[jump] && find !=0){

    if(head->tag == t_tag){
      find=0;
    }else{
      head =head->LRU_next;
      find++;
    }

  }
  *aux = head;

}

/************************************************************/
/************************************************************/

/*
  Flush is only used when we have write back, since we need to cache_block_size
  the dirty bit before reseting the cache
*/

void flush()
{
   if( cache_writeback){
     printf("\n Write_back FLush \n");
    if(cache_split){ //for SPLITTED, only c2(data) would have dirty bits

      Pcache_line current_line ;

      for(int i = 0; i < (c2.n_sets) ; i++){ // pasa por todo el cache

        if(c2.LRU_head[i] != NULL){ // there is something loaded in the set
            current_line = c2.LRU_head[i];
          for(int j= 1 ; j<= c2.set_contents[i]; j++){
           // gets the @ of the head of the set
            cache_stat_data.copies_back += words_per_block *(current_line-> dirty) ; //STATISTICS
            current_line = current_line->LRU_next; // JUMPS TO THE NEXT CACHE BLOCK IN THE SET
          } // end for of the set i

        } // if the set is not empty
    }// ends for of the cache, goes through all sets
  }else{ // for NON-SPLITTED cache -> c1
      Pcache_line current_line ;

      for(int i = 0; i < (c1.n_sets) ; i++){ // pasa por todo el cache

        if(c1.LRU_head[i] != NULL){ // there is something loaded in the set
          current_line = c1.LRU_head[i];

          for(int j= 1 ; j<= c1.set_contents[i]; j++){
             // gets the @ of the head of the set
            cache_stat_data.copies_back += (current_line-> dirty) * words_per_block; //STATISTICS
            current_line = current_line->LRU_next; // JUMPS TO THE NEXT CACHE BLOCK IN THE SET
          } // end for of the set i

        } // if the set is not empty
      }// ends for of the cache, goes through all sets

    } // ends ELSE of the splitted cache
  }// ends cache with CACHE_PARAM_WRITEBACK
} // ends FLUSH
/************************************************************/

/************************************************************/
void delete(head, tail, item)
  Pcache_line *head, *tail;
  Pcache_line item;
{
  if (item->LRU_prev) {
    item->LRU_prev->LRU_next = item->LRU_next;
  } else {
    /* item at head */
    *head = item->LRU_next;
  }

  if (item->LRU_next) {
    item->LRU_next->LRU_prev = item->LRU_prev;
  } else {
    /* item at tail */
    *tail = item->LRU_prev;
  }
}
/************************************************************/

/************************************************************/
/* inserts at the head of the list */
void insert(head, tail, item)
  Pcache_line *head, *tail;
  Pcache_line item;
{
  item->LRU_next = *head;
  item->LRU_prev = (Pcache_line)NULL;

  if (item->LRU_next)
    item->LRU_next->LRU_prev = item;
  else
    *tail = item;

  *head = item;
}
/************************************************************/

/************************************************************/
void dump_settings()
{
  printf("*** CACHE SETTINGS ***\n");
  if (cache_split) {
    printf("  Split I- D-cache\n");
    printf("  I-cache size: \t%d\n", cache_isize);
    printf("  D-cache size: \t%d\n", cache_dsize);
  } else {
    printf("  Unified I- D-cache\n");
    printf("  Size: \t%d\n", cache_usize);
  }
  printf("  Associativity: \t%d\n", cache_assoc);
  printf("  Block size: \t%d\n", cache_block_size);
  printf("  Write policy: \t%s\n",
	 cache_writeback ? "WRITE BACK" : "WRITE THROUGH");
  printf("  Allocation policy: \t%s\n",
	 cache_writealloc ? "WRITE ALLOCATE" : "WRITE NO ALLOCATE");
}
/************************************************************/

/************************************************************/
void print_stats()
{
  printf("\n*** CACHE STATISTICS ***\n");

  printf(" INSTRUCTIONS\n");
  printf("  accesses:  %d\n", cache_stat_inst.accesses);
  printf("  misses:    %d\n", cache_stat_inst.misses);
  if (!cache_stat_inst.accesses)
    printf("  miss rate: 0 (0)\n");
  else
    printf("  miss rate: %2.4f (hit rate %2.4f)\n",
	 (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses,
	 1.0 - (float)cache_stat_inst.misses / (float)cache_stat_inst.accesses);
  printf("  replace:   %d\n", cache_stat_inst.replacements);

  printf(" DATA\n");
  printf("  accesses:  %d\n", cache_stat_data.accesses);
  printf("  misses:    %d\n", cache_stat_data.misses);
  if (!cache_stat_data.accesses)
    printf("  miss rate: 0 (0)\n");
  else
    printf("  miss rate: %2.4f (hit rate %2.4f)\n",
	 (float)cache_stat_data.misses / (float)cache_stat_data.accesses,
	 1.0 - (float)cache_stat_data.misses / (float)cache_stat_data.accesses);
  printf("  replace:   %d\n", cache_stat_data.replacements);

  printf(" TRAFFIC (in words)\n");
  printf("  demand fetch:  %d\n", cache_stat_inst.demand_fetches +
	 cache_stat_data.demand_fetches);
  printf("  copies back:   %d\n", cache_stat_inst.copies_back +
	 cache_stat_data.copies_back);
}
/************************************************************/
