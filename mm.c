/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PAGES 8

typedef struct header {
    struct header *next;
    struct super_header *owner;
    int magic_num;
    int pad;
} __attribute__ ((packed, aligned(8))) header_t;

typedef struct super_header {
    struct super_header *non_full_super;
    struct super_header *next_free_4k;
    header_t *head_free;
    int free_count;
    int index;
    int size_class;
    int pad;
} __attribute__ ((packed, aligned(8))) super_header_t;

typedef struct class_head {
    int capacity;
    super_header_t *super_start;
} class_head_t;

class_head_t class_head[15];

static int num_4k = 0;
static int allocated_4k = 0;

super_header_t free_4k_start = {};

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    for (int i=0 ; i <= 14; i++) {
        class_head[i].capacity = (4096 * PAGES -sizeof(super_header_t)) / (sizeof(header_t) + (1 << (i + 4)));
        printf("size%i:%i\n",(1<<(i+4)),class_head[i].capacity );
    }
    return 0;
}

void print_class_head()
{
    for (int j = 0; j< 15; j++){
        printf("class head: %i: %p\n",j,class_head[j].super_start);
    }

}

static inline int find_class(size_t size)
{
    int size_class = __builtin_clzl(size) + __builtin_ctzl(size)+1 == sizeof(size)*8 ? __builtin_ctzl(size) :  sizeof(size)*8 - __builtin_clzl(size);
    if (size_class < 4)
        return 0;
    return size_class - 4;
}

void push_super_header(class_head_t *class_head,super_header_t *super_header)
{
    printf("push_super_header @ %p\n", super_header);
    super_header->non_full_super = class_head->super_start;
    class_head->super_start = super_header;
}

void pop_super_header(class_head_t *class_head)
{
    super_header_t *temp = class_head->super_start;
    class_head->super_start = class_head->super_start->non_full_super;
    temp->non_full_super = NULL;
}

void remove_super_header(class_head_t *class_head, super_header_t *super_header)
{
    printf("remove super_header @ %p\n", super_header);
    super_header_t *prev = NULL;
    super_header_t *cur = class_head->super_start;
    for (; cur && cur != super_header; prev = cur, cur = cur->non_full_super);
    
    if (cur == super_header) {
        if (prev) {
            prev->non_full_super = cur->non_full_super;
        } else {
            class_head->super_start = cur->non_full_super;
        }
    } else {
        printf("unable to find super_header @ %p\n", super_header);
    }
    
    cur->non_full_super = NULL;
    
    
    
    
    
/*    super_header_t *current = class_head->super_start;*/
/*    if (current == super_header){*/
/*        class_head->super_start = class_head->super_start->non_full_super;*/
/*        super_header->non_full_super = NULL;*/
/*    }*/
/*    else{*/
/*            while(current->non_full_super != super_header)*/
/*            {*/
/*                current = current->non_full_super;*/
/*            }*/
/*            current->non_full_super = super_header->non_full_super;*/
/*            super_header->non_full_super = NULL;*/
/*        }*/
}

void push_header(super_header_t *super_header,header_t *header)
{
    header->next = super_header->head_free;
    super_header->head_free = header;
    super_header->free_count++;
}

void pop_header(super_header_t *super_header)
{
/*    printf("pop_header\n");*/
/*    printf("head_free: %p\n",super_header->head_free);*/
    
    printf("pop header from %p\n", super_header);
    
    super_header->free_count--;
    header_t *temp = super_header->head_free;
    super_header->head_free = super_header->head_free->next;
    temp->next = NULL;
    if (super_header->free_count == 0){
        int size_class = super_header->size_class;
        remove_super_header(&class_head[size_class],super_header);
    }
}

void insert_free_4k(super_header_t *free_4k)
{
    allocated_4k--;
    if (free_4k_start.next_free_4k == NULL || free_4k->index > free_4k_start.next_free_4k->index)
    {
        free_4k = free_4k_start.next_free_4k;
        free_4k_start.next_free_4k = free_4k;
    }
    else
    {
        super_header_t *current = free_4k_start.next_free_4k;
        while (current ->next_free_4k != NULL || free_4k->index < current->next_free_4k->index)
        {
            current = current ->next_free_4k;
        }
        free_4k->next_free_4k = current->next_free_4k;
        current ->next_free_4k = free_4k;
    }
}

void remove_free_4k(super_header_t *free_4k)
{
    allocated_4k++;
    if (free_4k_start.next_free_4k == free_4k)
    {
        free_4k_start.next_free_4k = NULL;
    }
    super_header_t *prev = &free_4k_start;
    super_header_t *next = free_4k_start.next_free_4k;    
    while(1)
    {
        if (next == free_4k)
        {
            prev->next_free_4k = next;
            free_4k->next_free_4k = NULL;
        }
    }
}

void free_4k()
{
    if (!allocated_4k) {
        super_header_t *current = &free_4k_start;;
        super_header_t *temp;
        while(current->next_free_4k)
        {
            temp = current->next_free_4k;
            current->next_free_4k = NULL;
            current = temp;
        }
        printf("free 4k\n");
    }
    
    
    
    
/*    printf("free_4k\n");*/

}

super_header_t *find_last_4k()
{
    if (free_4k_start.next_free_4k !=NULL){
        super_header_t *current = free_4k_start.next_free_4k; 
        while(current ->next_free_4k != NULL)
        {
            current = current->next_free_4k;
        }
        return current;
    }
    else
    {
        return NULL;
    }
    
}



void init_4k(super_header_t *super_header,int size_class)
{
/*    printf("start initialize 4k\n class = %i\n",size_class);*/
/*    printf("capacity:%i\n",class_head[size_class].capacity);*/
    for (int i = 0; i < class_head[size_class].capacity; i++)
    {
        header_t *header = (header_t *)((void *)(super_header+1) + i*(sizeof(header_t)+(1<<(4+size_class))));
        header->owner = super_header;
        push_header(super_header,header);
        header->magic_num = 0xbeef;
    }
}

void *alloc_4k(int size_class)
{
    super_header_t *super_header;
    if (free_4k_start.next_free_4k) {
        printf("*used \n");
        super_header = find_last_4k();
    } else {
        printf("allocate new 4k\n");
        void *p = mem_sbrk(4096 * PAGES);
        if (p == (void *)-1) {
            return NULL;
        }/*            printf("full\n");*/
        super_header = (super_header_t *)(p);
    }
    init_4k(super_header, size_class);
    push_super_header(&class_head[size_class],super_header);
    super_header->index = ++num_4k;
    super_header->size_class = size_class;
    allocated_4k++;
    return super_header;
}

 /* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size)
{
/*    printf("***malloc size:%i\n",size);*/
    if (!size)
        return NULL;
    
    int size_class = find_class(size);
    printf("**allocate size:%i\n",size_class);
    printf("class head:%p\n",class_head[size_class].super_start);
    if (!class_head[size_class].super_start) {
        alloc_4k(size_class);
    }
    
    header_t *block = class_head[size_class].super_start->head_free;
    pop_header(class_head[size_class].super_start);
    printf("4k in use:%i\n",allocated_4k);
    return (void *)(block + 1);
    
/*/*    printf("sizeclass:%i\n",size_class);*/
/*    if (class_head[size_class].super_start) {*/
/*/*        printf("**Suse existed\n");*/
/*        header_t *block = class_head[size_class].super_start->head_free;*/
/*        pop_header(class_head[size_class].super_start);*/
/*        return (void *)(block +1);*/
/*    }*/
/*        */
/*    else{*/
/*/*        printf("**use new\n");*/
/*        alloc_4k(size_class);*/
/*/*        printf("head_ free: %p\n",class_head[size_class].super_start->head_free);*/
/*        header_t *block = class_head[size_class].super_start->head_free;*/
/*        pop_header(class_head[size_class].super_start);*/
/*        return (void *)(block +1);*/
/*    }*/
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    header_t *header = ptr - sizeof(header_t);
    if (header->magic_num == 0xbeef) {
        super_header_t *super_header = header->owner;
        push_header(super_header, header);
        int size_class = super_header->size_class;
        printf("**free size:%i\n",size_class);
        //--super_header->usage;
        //printf("usage: %i\n",super_header->usage);
        
/*becomes empty*/
/*was nonfull*/

        if (super_header->free_count == class_head[size_class].capacity){
            /*was nonfull*/
            if (class_head[size_class].capacity != 1) {
                remove_super_header(&class_head[size_class],super_header);
            }
            insert_free_4k(super_header);
            free_4k();
        }
        else {
            if (super_header->free_count == 1 ){
                push_super_header(&class_head[size_class],super_header);
            }
        }

/*if the 4k block was full*/
/*        else if (super_header->free_count == 1 && class_head[size_class].capacity != 1)*/
/*        {*/
/*            push_super_header(&class_head[size_class],super_header);*/
/*        }*/
        
    }
    
    else {
/*        printf("The input pointer is wrong!");*/
        exit(-1);
    }
    
    printf("4k in use:%i\n",allocated_4k);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
/*void *mm_realloc(void *ptr, size_t size)*/
/*{*/
/*    void *oldptr = ptr;*/
/*    void *newptr;*/
/*    size_t copySize;*/
/*    */
/*    newptr = mm_malloc(size);*/
/*    if (newptr == NULL)*/
/*      return NULL;*/
/*    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);*/
/*    if (size < copySize)*/
/*      copySize = size;*/
/*    memcpy(newptr, oldptr, copySize);*/
/*    mm_free(oldptr);*/
/*    return newptr;*/
/*}*/
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/*int main()*/
/*{*/
/*    mm_init();*/
/*    void *p = mm_malloc(2040);*/
/*}*/















