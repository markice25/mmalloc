#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <math.h>



/*test whether intializing pointer to NULL is necessary*/
/*int *header[8];*/

/*int main() */
/*{   */
/*    int i;*/
/*    for (i = 0; i<=7; i++){*/
/*        if (header[i] == NULL);*/
/*            printf("%d is null point",i);*/
/*    }*/
/*} */

/*test 4k allocator*/

typedef struct header{
    struct header *next;
} header_t;

typedef struct super_header {
    int usage;
    struct super_header *non_full_super;
    header_t *head_free;
} super_header_t;

header_t *head[7] = {NULL};
int capacity[7];


void *alloc_4k()
{
    super_header_t *header;
    void *p = (int *) sbrk(4096);
    if (p == (void *)-1)
        return NULL;
    else{
        header = p;
        
        return (void *)(header + 1);
    }
}

/*int main(){*/
/*    void *p1;*/
/*    p1 = alloc_4k();*/
/*    void *p2; */
/*    p2 = alloc_4k();*/
/*    printf("%p\n",p1);*/
/*    printf("%p\n",p2);*/
/*    printf("%ld",p2-p1);*/
/*    printf()*/
/* }*/

int find_class(size_t size)
{
    int size_class = __builtin_clzl(size) + __builtin_ctzl(size)+1 == sizeof(size)*8 ? __builtin_ctzl(size) :  sizeof(size)*8 - __builtin_clzl(size);
    if (size_class < 4)
        return 0;
    return size_class-4;
}

void push_super_head(super_header_t **class_head,super_header_t *super_header)
{
    super_header_t *temp;

    if (*class_head==NULL)
    {
        *class_head = super_header;
        super_header->non_full_super = NULL;
        printf("here\n");
    }
    else
    {
        temp = *class_head;
        *class_head = super_header;
        super_header->non_full_super = temp;
        printf("there\n");
    }
}

void pop_super_head(super_header_t **class_head)
{
    *class_head = (*class_head)->non_full_super;
}

/*int main() */
/*{*/
/*    */
/*    for(int i ; i<=6; i++)*/
/*    {*/
/*        capacity[i] = (4096-sizeof(super_header_t))/(sizeof(header_t)+(1<<(i+4)));*/
/*        printf("%i\n",capacity[i]);*/
/*    }*/
/*    return 0;*/
/*}*/


int main()
{
    super_header_t **class_head = (super_header_t **)malloc(sizeof(void *));
    super_header_t *header1= (super_header_t *)malloc(sizeof(super_header_t));
    super_header_t *header2= (super_header_t *)malloc(sizeof(super_header_t));
    super_header_t *header3= (super_header_t *)malloc(sizeof(super_header_t));
/*    super_header_t *class_head ;*/
/*    class_head = (super_header_t *)malloc(sizeof(super_header_t));*/
/*    super_header_t *header1;*/
/*    super_header_t *header2;*/
/*    super_header_t *header3;*/
    printf("head:%p\n",class_head);
    printf("header1:%p\n",header1);
    printf("header2:%p\n",header2);
    printf("header3:%p\n",header3);
    header1->usage = 1;
    header2->usage = 2;
    header3->usage = 3;
    push_super_head(class_head,header1);
    printf("head:%p\n",*class_head);
    push_super_head(class_head,header2);
    push_super_head(class_head,header3);
    printf("header1:%p\n",(*class_head)->non_full_super);
    printf("header2:%p\n",(*class_head)->non_full_super->non_full_super);
    printf("header3:%p\n",(*class_head)->non_full_super->non_full_super->non_full_super);
    printf("header1:%i\n",(*class_head)->usage);
    printf("header2:%i\n",(*class_head)->non_full_super->usage);
    printf("header3:%i\n",(*class_head)->non_full_super->non_full_super->usage);
    
    pop_super_head(class_head);
    printf("header1:%i\n",(*class_head)->usage);
    pop_super_head(class_head);
    printf("header1:%i\n",(*class_head)->usage);
    pop_super_head(class_head);
    printf("header1:%p\n",(*class_head));
    
    push_super_head(class_head,header1);
    printf("head:%p\n",*class_head);
    push_super_head(class_head,header2);
    push_super_head(class_head,header3);
    printf("header1:%p\n",(*class_head)->non_full_super);
    printf("header2:%p\n",(*class_head)->non_full_super->non_full_super);
    printf("header3:%p\n",(*class_head)->non_full_super->non_full_super->non_full_super);
    printf("header1:%i\n",(*class_head)->usage);
    printf("header2:%i\n",(*class_head)->non_full_super->usage);
    printf("header3:%i\n",(*class_head)->non_full_super->non_full_super->usage);


    
}
