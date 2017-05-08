
/*
 * File: cmap.c
 * Author: YOUR NAME HERE
 * ----------------------
 *
 */

#include "cmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>


// a suggested value to use when given capacity_hint is 0
#define DEFAULT_CAPACITY 1023

/* Type: struct CMapImplementation
 * -------------------------------
 * This definition completes the CMap type that was declared in
 * cmap.h. You fill in the struct with your chosen fields.
 */
typedef struct CMapImplementation {
    void **buckets;
    int nbuckets;
    int valsz;
    int nelems;
    CleanupValueFn cleanup;
}CMap;






/* Function: hash
 * --------------
 * This function adapted from Eric Roberts' _The Art and Science of C_
 * It takes a string and uses it to derive a "hash code," which
 * is an integer in the range [0..nbuckets-1]. The hash code is computed
 * using a method called "linear congruence." A similar function using this
 * method is described on page 144 of Kernighan and Ritchie. The choice of
 * the value for the multiplier can have a significant effort on the
 * performance of the algorithm, but not on its correctness.
 * The computed hash value is stable, e.g. passing the same string and
 * nbuckets to function again will always return the same code.
 * The hash is case-sensitive, "ZELENSKI" and "Zelenski" are
 * not guaranteed to hash to same code.
 */
static int hash(const char *s, int nbuckets)
{
   const unsigned long MULTIPLIER = 2630849305L; // magic number
   unsigned long hashcode = 0;
   for (int i = 0; s[i] != '\0'; i++)
      hashcode = hashcode * MULTIPLIER + s[i];
   return hashcode % nbuckets;
}
//Sets the next pointer of the cell
static void setNext(void **cell, void *next)
{
    *cell=next;
}
//gets the next cell
static void *getNext(void **cell)
{
    void *next=*cell;
    return next;
}
//sets the key for the given cell
static void setKey(void *cell, const char *key)
{
    void *keyAddr = (char *)cell+sizeof(void *);
    memcpy(keyAddr, key, strlen(key)+1);
}
//returns the key for a cell
static char *getKey(void *cell)
{
   return (char *)cell+sizeof(void *);
}
//sets a cell value
static void setValue(CMap *cm, void *cell, const char *key, const void *value)
{
    void *valAddr=(char *)cell+sizeof(void *)+strlen(key)+1;
    memcpy(valAddr, value, cm->valsz);
}
//gets cell value
static void *getValue(void *cell, const char *key)
{
    return (void *)((char *)cell+sizeof(void *)+strlen(key)+1);
}

//Mallocs a cell and initializes the pointer, key, and value.
static void *createCell(CMap *cm, const char *key, const void *addr)
{
    void *cell=malloc(sizeof(void *)+strlen(key)+1+cm->valsz);
    assert(cell != NULL); 
    setNext(cell, NULL);
    setKey(cell, key);
    setValue(cm, cell, key, addr);
    cm->nelems++;
    return cell;
}
//Checks if a bucket contains a given key.
static void *containsKey(void **curBucket, const char* key)
{
    void *cell = *curBucket;
    while(true){
	if(strcmp(key, getKey(cell))==0) return cell;
	cell=getNext(cell);
        if(cell==NULL) break;
    }
    return cell;
}
//Gets the last cell in a bucket
static void *getEnd(CMap *cm, void **bucket)
{
    void **cell=*bucket;
    while(*cell != NULL) cell=getNext(cell);
    return cell;
}

//Gets the bucket at the given index from the Map
static void** getBucket(const CMap *cm, int bucket)
{
    return (void **)((char *)cm->buckets+sizeof(void *) * bucket);
}


//Gets the previous cell(or bucket) for a given key.
static void **getPrevious(void **curBucket, const char *key)
{
    void **cell= curBucket;
    void **next=*curBucket;
    while(next !=NULL){
	if(strcmp(key, getKey(next))==0) return cell;
	cell=getNext(cell);
	next=getNext(next);
    }
    return NULL;
} 
//Finds the next valid Bucket, used when iterating through keys.
static void **nextValidBucket(const CMap *cm, int startBucket)
{
    void **bucket = getBucket(cm, startBucket);
    int addition=0;
    while(*bucket==NULL && startBucket+addition+1<cm->nbuckets){
	addition++;
	bucket=getBucket(cm, startBucket+addition);
    }
    return bucket;
}

CMap *cmap_create(size_t valuesz, size_t capacity_hint, CleanupValueFn fn)
{
    assert(valuesz > 0);
    CMap *cm=malloc(sizeof(CMap));
    assert(cm != NULL);
    int capacity=capacity_hint==0 ? DEFAULT_CAPACITY : capacity_hint;
    cm->buckets=calloc(capacity, sizeof(void *));
    cm->nbuckets=capacity;
    cm->valsz=valuesz; 
    cm->cleanup=fn; 
    return cm;
}
//Frees an individuall cell, calls cleanup if necessary, and adjusts pointers.
void freeCell(CMap *cm, const char* key)
{
    void **bucket = getBucket(cm, hash(key, cm->nbuckets));
    void *cell=containsKey(bucket, key);
    void *previous=getPrevious(bucket, key);
    void *next=getNext(cell);
    if(cm->cleanup != NULL) cm->cleanup(getValue(cell, key));
    free(cell);
    setNext(previous, next);
}

void cmap_dispose(CMap *cm)
{   
    const char* key = cmap_first(cm);
    while(key != NULL){
	const char* next=cmap_next(cm, key);
	freeCell(cm, key);
	key=next;
    }
    free(cm->buckets);
    free(cm);

}

int cmap_count(const CMap *cm)
{ 
    return cm->nelems;
}

void cmap_put(CMap *cm, const char *key, const void *addr)
{
    int correctBucket = hash(key, cm->nbuckets);
    void **curBucket = getBucket(cm, correctBucket);
    void **cell;
    if(*curBucket==NULL){
	cell= createCell(cm, key, addr);
	setNext(curBucket, cell);
	return;
    }
    cell=containsKey(curBucket, key);
    if(cell != NULL){
	 setValue(cm, cell, key, addr);
	 return;
    }
    cell=createCell(cm, key, addr);
    void *prevCell=getEnd(cm, curBucket);
    setNext(prevCell, cell);
}

void *cmap_get(const CMap *cm, const char *key)
{
    int correctBucket = hash(key, cm->nbuckets);
    void **curBucket = getBucket(cm, correctBucket);
    if(*curBucket == NULL) return NULL;
    void **cell=containsKey(curBucket, key);
    if(cell == NULL) return NULL;
    void *value = getValue(cell, key);
    return value;  
}

void cmap_remove(CMap *cm, const char *key)
{
    int correctBucket = hash(key, cm->nbuckets);
    void **curBucket = getBucket(cm, correctBucket);
    if(*curBucket==NULL) return;
    void **cell=containsKey(curBucket, key);
    if(cell==NULL) return;
    freeCell(cm, key);
    cm->nelems--;
}

const char *cmap_first(const CMap *cm)
{ 
    void **bucket = nextValidBucket(cm, 0);
    if(*bucket == NULL) return NULL;
    return getKey(*bucket);
}

const char *cmap_next(const CMap *cm, const char *prevkey)
{ 
    int prevBucket = hash(prevkey, cm->nbuckets);
    void **curBucket = getBucket(cm, prevBucket);
    void **cell=containsKey(curBucket, prevkey);
    if(getNext(cell) != NULL) return getKey(getNext(cell));
    curBucket = nextValidBucket(cm, prevBucket+1);
    if(*curBucket == NULL) return NULL;
    return getKey(*curBucket);
}
