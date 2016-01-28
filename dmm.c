#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */


#define IS_ALLOC(x)  (((x) -> size) & 1)
#define GET_SIZE(x) (((x) -> size) & (~7))
#define SET_SIZE(x,a) ( (x) -> size =  ( (a) & (~7) ) | ( (x) -> size & 0x7) )
#define FREE(x) ((x) -> size &=  ~1 )
#define ALLOC(x) ((x) -> size |= 1)
#define MOVE(x,n) ((void*)(x) +(n))


#define EXTEND_UNIT (MAX_HEAP_SIZE)

#define OVERHEAD (2*sizeof(footer_t))

#define MINSIZE (sizeof(metadata_t) + sizeof(footer_t))

typedef struct metadata {
       /* size_t is the return type of the sizeof operator. Since the size of
 	* an object depends on the architecture and its implementation, size_t 
	* is used to represent the maximum size of any object in the particular
 	* implementation. 
	* size contains the size of the data object or the amount of free
 	* bytes 
	*/
	size_t size;
	struct metadata* next;
	struct metadata* prev; //What's the use of prev pointer?
} metadata_t;

typedef struct footer{
    size_t size;
}footer_t;


/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

static metadata_t* freelist = NULL;

static size_t total_size = 0;

static void* mbrk =NULL;

static size_t current = 0;
static bool is_init = false;

void add_to_list(metadata_t * cur){
    if(cur == NULL)
        return ;
     
    FREE(cur);    
    footer_t* t = (footer_t *)MOVE(cur,GET_SIZE(cur)-sizeof(footer_t));
    SET_SIZE(t,GET_SIZE(cur));
    FREE(t);
    
    if(freelist == NULL){
        freelist = cur;
        freelist -> prev = NULL;
        freelist -> next = NULL;
    }else{
        metadata_t * a = freelist;
        freelist -> prev = cur;
        freelist = cur ;
        freelist -> next =a;
        freelist -> prev = NULL;
    }
    
    
}
void delete_from_list(metadata_t * cur){
    if(cur == NULL)
        return;
        
    if(cur -> prev == NULL && cur-> next == NULL)
        freelist = NULL;
        
    else if(cur ->prev == NULL){
        freelist = cur->next;
        freelist ->prev = NULL;
    }
    else if(cur -> next == NULL)
        cur->prev ->next =NULL;
    else{
        
        cur->prev ->next = cur ->next;
        cur->next-> prev = cur -> prev;
    }
}

bool extend_memory(size_t size){
    size_t s;
    
    if(size % EXTEND_UNIT == 0)
        s = size;
    else s = (size / EXTEND_UNIT + 1) * EXTEND_UNIT;
        
    if(s + current > total_size )
        return false;
    
    void* ans = mbrk;
    mbrk = mbrk + s;
    
    
    metadata_t* tmp = (void *) ans;
    SET_SIZE(tmp,s);    
    tmp ->prev =NULL;
    
    add_to_list(tmp);
    
    return true;
}

metadata_t* split(metadata_t * cur,size_t size){
    if(cur == NULL || GET_SIZE(cur) < MINSIZE +size )
        return NULL;
        
    size_t second_size = GET_SIZE(cur) - size;
    
    
    SET_SIZE(cur, size );
    footer_t* t = (footer_t*)MOVE(cur, GET_SIZE(cur) - sizeof(footer_t));   
    SET_SIZE(t , size);
    
    metadata_t * second = (metadata_t *)MOVE(t,sizeof(footer_t));
    SET_SIZE(second, second_size);
    
    t = (footer_t*)MOVE(second, GET_SIZE(second) - sizeof(footer_t));      
    SET_SIZE(t,second_size);
    
    return second;
}

// first fit
void *find_fit(size_t size){

    if(size < MINSIZE - OVERHEAD )
        size = MINSIZE -OVERHEAD ;
        
    metadata_t* cur = freelist;
    void * ans = NULL;
    long long unsigned min = 100000000000;
    while(cur){
        if(GET_SIZE(cur) >= size + OVERHEAD){
            if(min > GET_SIZE(cur) - size){
                ans = (void*)cur;
                min = GET_SIZE(cur) - size;
            }
        }
        
        cur = cur -> next;
    }
    printf("%d\n",min);
    if(ans != NULL){
        delete_from_list((metadata_t*)ans);
        add_to_list(split((metadata_t*)ans,size + OVERHEAD));
        
        ALLOC((metadata_t*)ans);
        footer_t* t = (footer_t*)MOVE(ans, GET_SIZE((metadata_t*)ans) - sizeof(footer_t));
        ALLOC(t);
        ans += sizeof(footer_t);
    }
    
    return ans;
}


void* dmalloc(size_t numbytes) {

    if(!is_init){
        dmalloc_init();
    }
    
	assert(numbytes > 0);

    size_t size = ALIGN(numbytes);
    void * ans = find_fit(size);
    
	if(ans == NULL){
	    if(!extend_memory(size))
	        return NULL;	        
	     ans = find_fit(size);
	}
	
	return ans;
}
metadata_t* coalescing(metadata_t* cur){
    footer_t* tail = (footer_t*)MOVE(cur,GET_SIZE(cur)-sizeof(footer_t));
    
    footer_t* prev_tail = (footer_t *) MOVE(cur, -sizeof(footer_t));
    
    metadata_t * ans= cur;
    
    size_t size = GET_SIZE(cur);
    
    /* prev is free*/
    if(!IS_ALLOC(prev_tail)){
        size+= GET_SIZE(prev_tail);
        ans = (metadata_t *)MOVE(cur,-GET_SIZE(prev_tail));
        delete_from_list(ans);
    }
    
    
    /*not last block*/
    if(mbrk + current != ((void*)tail) + sizeof(footer_t)){
        metadata_t* next_head = (metadata_t*)MOVE(cur,GET_SIZE(cur));
        if(!IS_ALLOC(next_head)){
            size += GET_SIZE(next_head);
            delete_from_list(next_head);
            
        }
    }
    
    SET_SIZE(ans,size);
    footer_t * t = (footer_t *)MOVE(ans,GET_SIZE(ans) - sizeof(footer_t));
    SET_SIZE(t,size);
    
    
    return ans;
}

void dfree(void* ptr) {

	/* Your free and coalescing code goes here */
	metadata_t * cur = (metadata_t *)MOVE(ptr, -sizeof(footer_t));
	FREE(cur);
	footer_t * t = (footer_t *)MOVE(cur,GET_SIZE(cur)-sizeof(footer_t));
	FREE(t);
	
	add_to_list(coalescing(cur));
	
	
}


bool dmalloc_init() {

	/* Two choices: 
 	* 1. Append prologue and epilogue blocks to the start and the end of the freelist
 	* 2. Initialize freelist pointers to NULL
 	*
 	* Note: We provide the code for 2. Using 1 will help you to tackle the
 	* corner cases succinctly.
 	*/
    is_init = true;
	size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
	total_size = max_bytes;
    mbrk = (metadata_t*) sbrk(max_bytes); // returns heap_region, which is initialized to freelist
    
	/* Q: Why casting is used? i.e., why (void*)-1? */
	if (mbrk == (void *)-1)
		return false;

    current = EXTEND_UNIT;
    (*(size_t *)mbrk) = 1;
    
    freelist = (metadata_t *)(mbrk + sizeof(size_t));
    freelist ->next = NULL;
    freelist -> prev = NULL;
    SET_SIZE(freelist, EXTEND_UNIT - sizeof(size_t));
    FREE(freelist);

    
    footer_t* t = (footer_t *)MOVE(freelist,GET_SIZE(freelist)-sizeof(footer_t));
    SET_SIZE(t,GET_SIZE(freelist));
    FREE(t);
	return true;
}

/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
void print_freelist() {
	metadata_t *freelist_head = freelist;
	while(freelist_head != NULL) {
		DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",freelist_head->size,freelist_head,freelist_head->prev,freelist_head->next);
		//printf("%d\n", freelist_head->size);
		freelist_head = freelist_head->next;
		
	}
	DEBUG("\n");
}
