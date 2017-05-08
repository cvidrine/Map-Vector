
/*
 * File: cvector.c
 * Author: Charles Vidrine
 * ----------------------
 *
 */

#include "cvector.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <search.h>

// a suggested value to use when given capacity_hint is 0
#define DEFAULT_CAPACITY 16

/* Type: struct CVectorImplementation
 * ----------------------------------
 * This definition completes the CVector type that was declared in
 * cvector.h. You fill in the struct with your chosen fields.
 */
struct CVectorImplementation {
    void *array;
    int nelems;
    size_t elemsz;
    size_t allocated;
    size_t used;
    CleanupElemFn fn;
};

//Checks if an index is valid.
void validIndex(const CVector *cv, int index)
{
   assert(index >= 0 && index <= cv->nelems);
}
//Checks if the method is initialized.
bool is_initialized(const CVector *cv)
{
    return cv!=NULL;
}

//Get the nth element from the array.
void *get_nth(const CVector *cv, int index)
{
   void *nth= (char *)(cv->array)+index * cv->elemsz;
   return nth;
}

//Shifts the array pointers over if something is added.
void shiftAdded(CVector *cv, int index)
{
    for(int i=cv->nelems-1; i>index; i--){
	void *currElem=get_nth(cv, i);
	void *otherElem=get_nth(cv, i-1);
	memmove(currElem, otherElem, cv->elemsz);
    }
}
//Shifts the array pointers if something is removed.
void shiftRemoved(CVector *cv, int index)
{
    for(int i=index+1; i<cv->nelems; i++){
	void *currElem=get_nth(cv, i);
	void *otherElem=get_nth(cv, i-1);
	memmove(otherElem, currElem, cv->elemsz);
    }
}

void addElement(CVector *cv)
{
    cv->nelems++;
    cv->used+=cv->elemsz;
    if(cv->used > cv->allocated){
	cv->allocated*=2;
	cv->array=realloc(cv->array, cv->allocated);
	assert(cv->array != NULL);
    }
}


CVector *cvec_create(size_t elemsz, size_t capacity_hint, CleanupElemFn fn)
{
  assert(elemsz > 0);
  CVector *cv=malloc(sizeof(CVector));
  assert(is_initialized(cv));
  int capacity = capacity_hint==0 ? DEFAULT_CAPACITY : capacity_hint;
  cv->array= calloc(capacity, elemsz);
  cv->allocated=capacity*elemsz;
  cv->used=0;
  cv->nelems=0;
  cv->elemsz=elemsz;
  cv->fn = fn;
  return cv;
}

void cvec_dispose(CVector *cv)
{ 
    if(cv->fn != NULL) for(void *cur=cvec_first(cv); cur !=NULL; cur = cvec_next(cv, cur)) cv->fn(cur);
    free(cv->array);
    free(cv);
}


int cvec_count(const CVector *cv)
{ 
    return cv->nelems;
}

void *cvec_nth(const CVector *cv, int index)
{
 validIndex(cv, index);
 void *nth = get_nth(cv, index);
 return nth;
}

void cvec_insert(CVector *cv, const void *addr, int index)
{ 
  validIndex(cv, index); //assert for valid index
  addElement(cv); //allocates memory and shifts member variables
  shiftAdded(cv, index); //shifts indices
  void *nth=get_nth(cv, index); //pointer to index to change
  memmove(nth, addr, cv->elemsz); //copies information into cvec.
}

void cvec_append(CVector *cv, const void *addr)
{ 
    addElement(cv);
    memmove(get_nth(cv, (cv->nelems)-1), addr, cv->elemsz); 
}

void cvec_replace(CVector *cv, const void *addr, int index)
{ 
    validIndex(cv, index);
    void *nth=get_nth(cv, index);
    if(cv->fn != NULL) cv->fn(nth);
    memmove(nth, addr, cv->elemsz);
}

void cvec_remove(CVector *cv, int index)
{ 
   validIndex(cv, index);
   void *nth=get_nth(cv, index);
   if(cv->fn !=NULL) cv->fn(nth);
   shiftRemoved(cv, index);
   cv->nelems--;
}

int cvec_search(const CVector *cv, const void *key, CompareFn cmp, int start, bool sorted)
{
    assert(start>=0 && start < cv->nelems);
    void *elem=NULL;
    size_t elemSearch=cv->nelems-start;
    void *base= get_nth(cv, start);
    if(sorted){
	elem=bsearch(key, base, elemSearch, cv->elemsz, cmp);
    }else{
	elem=lfind(key, base, &elemSearch, cv->elemsz, cmp);
    }
    if(elem==NULL) return -1;
    int difference = (unsigned long long)elem-(unsigned long long)cv->array;
    int index=difference/cv->elemsz;
    return index; 
}

void cvec_sort(CVector *cv, CompareFn cmp)
{
    qsort(cv->array, cv->nelems, cv->elemsz, cmp);
}

void *cvec_first(const CVector *cv)
{   
    return get_nth(cv, 0);
}

void *cvec_next(const CVector *cv, const void *prev)
{ 
    void *next=(char *)prev+cv->elemsz;
    void *last=get_nth(cv, cv->nelems-1);
    if((char *)next-(char *)last > 0) return NULL;
    return next;
}
