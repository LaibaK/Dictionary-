#include<stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "libDict.h"

#define DEBUG
#define DEBUG_LEVEL 3

#ifdef DEBUG
# define DEBUG_PRINT(x) printf x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif

#define DICT_INIT_ROWS 1024 
#define DICT_GROW_FACTOR 2
#define ROW_INIT_ENTRIES 8
#define ROW_GROW_FACTOR 2 

#define PRIME1 77933 // a large prime
#define PRIME2 119557 // a large prime

/**
 * hash *c as a sequence of bytes mod m
 */
int dictHash(char *c, int m){
	int sum=0;
	while(*c!='\0'){
		int num = *c; 
		sum+= PRIME1*num+PRIME2*sum;
		c++;
	}
	if(sum<0)sum=-sum;
	sum = sum%m;
	return sum;
}

/**
 * Print the dictionary, 
 * level==0, dict header
 * level==1, dict header, rows headers
 * level==2, dict header, rows headers, and keys
 */
void dictPrint(Dict *d, int level){
	if(d==NULL){
		printf("\tDict==NULL\n");
		return;
	}
	printf("Dict\n");
	printf("\tnumRows=%d\n",d->numRows);
	if(level<1)return;

	for(int i=0;i<d->numRows;i++){
		printf("\tDictRow[%d]: numEntries=%d capacity=%d keys=[", i, d->rows[i].numEntries, d->rows[i].capacity);
		if(level>=2){
			for(int j=0;j<d->rows[i].numEntries;j++){
				printf("%s, ",d->rows[i].entries[j].key);
			}
		}
		printf("]\n");
	}
}

/**
 * Return the DictEntry for the given key, NULL if not found.
 * This is so we can store NULL as a value.
 */
DictEntry *dictGet(Dict *d, char *key){

	// find row
	int h = dictHash(key, d->numRows);
    	DictRow *row = &(d->rows[h]);

	// find key in row
	for (int i = 0; i < row->numEntries; i++) {
        	DictEntry *entry = &(row->entries[i]);
        	if (strcmp(entry->key, key) == 0) {return entry;}
	}
	return NULL;
}

/**
 * Delete key from dict if its found in the dictionary
 * Returns 1 if found and deleted
 * Returns 0 otherwise
 */
int dictDel(Dict *d, char *key){
	//find row
	int h=0;
        h = dictHash(key, d->numRows);
    	DictRow *row = &(d->rows[h]);	

	#ifdef DEBUG
	printf("dictDel(d,%s) hash=%d\n",key, h);
	dictPrint(d,DEBUG_LEVEL);
	#endif

	// find key in row
	// free key
	// Move everything over
	int found = 0; 
	int loc; 
	for (int i = 0; i < row->numEntries; i++) {
        	DictEntry *entry = &(row->entries[i]);
        	if (strcmp(entry->key, key) == 0) {
            		loc = i;
			free(entry->key); // free the key string  
                        found = 1;  
           	}
        }

	if (found == 0){return 0;}
	
	//shift entries down 
	for (int i = loc; i < row->numEntries - 1; i++) {
                row->entries[i].key = row->entries[i+1].key;
		row->entries[i].value = row->entries[i+1].value;

    	}

	    
    
    	row->numEntries--;
    	//free(entry->key); // free the key string
    	//free(entry); // free the entry

	#ifdef DEBUG
	dictPrint(d,DEBUG_LEVEL);
	#endif

	return 1;



}

/** 
 * put (key, value) in Dict
 * return 1 for success and 0 for failure
 */
int dictPut(Dict *d, char *key, void *value){
	//This is the hash index
	int h = dictHash(key, d->numRows);

	#ifdef DEBUG
	printf("dictPut(d,%s) hash=%d\n",key, h);
	dictPrint(d,DEBUG_LEVEL);
	#endif

	// If key is already here, just replace value
	//Use DictGet to  check if key is in already in Dict
	DictEntry *check = dictGet(d, key);
	if  (check != NULL) {
		check->value = value;
		return 1;
	}
 

	#ifdef DEBUG
	dictPrint(d,DEBUG_LEVEL);
	#endif

	/** 
	 * else we need to place (key,value) as a new entry in this row
	 * if there is no space, expand the row
	 */

	//Retrieve row using hash index 
	DictRow *row = &(d->rows[h]);

	//check if row is full
	if (row->numEntries == row->capacity){
		//new capacity is doubled
        	int newCapacity = row->capacity * 2;
		//allocate new space
        	DictEntry *newEntries = realloc(row->entries, newCapacity * sizeof(DictEntry));
        	if (newEntries == NULL) {return 0;}
        	//adjust capcity and where entries points
		row->capacity = newCapacity;
        	row->entries = newEntries;
    	}

	#ifdef DEBUG
	dictPrint(d,DEBUG_LEVEL);
	#endif


	/** 
	 * This is a new key for this row, so we want to place the key, value pair
	 * In python only immutables can be hash keys. If the user can change the key sitting
	 * in the Dict, then we won't be able to find it again. We solve this problem here
	 * by copying keys using strdup.
	 * 
	 * At this point we know there is space, so copy the key and place it in the row
	 * along with its value.
	 */

	//identify where the new entry points to	
    	DictEntry *newEntry = &(row->entries[row->numEntries]);
    	newEntry->key = strdup(key);
    	newEntry->value = value;
   	row->numEntries++;
  
	#ifdef DEBUG
	dictPrint(d,DEBUG_LEVEL);
	#endif




	return 1; 
}

/**
 * free all resources a llocated for this Dict. Everything, and only those things
 * allocated by this code should be freed.
 */
void dictFree(Dict *d){
	// Free DictEntries
	for (int i = 0; i < d->numRows; i++){
		DictRow	*row = &(d->rows[i]);
		//Free DictEntry
		for (int j = 0; j < row->numEntries; j++){
			DictEntry *entry = &(row->entries[j]);
			free(entry->key);
		}
	free(row->entries);
		
	}
	//free DictRow and Dict
	free(d->rows);
	free(d);
}

/**
 * Allocate and initialize a new Dict. Initially this dictionary will have initRows
 * hash slots. If initRows==0, then it defaults to DICT_INIT_ROWS
 * Returns the address of the new Dict on success
 * Returns NULL on failure
 */
Dict * dictNew(int initRows){
	Dict *d=NULL;
	if((d=malloc(sizeof(Dict)))==NULL)return NULL;
	
	//If initRows = 0, set default amount 
	if(initRows == 0){initRows = DICT_INIT_ROWS;} 

	//set Dict num rows
	d->numRows = initRows;

	//Allocate space for DictRows *rows
	if((d->rows=malloc(initRows * sizeof(DictRow)))==NULL){
	free(d);	
	return NULL;
	}

	//Allocate space for DictEntry *entries
	for (int i = 0; i < initRows; i++) {
		//set pointer *row to element in row
		DictRow *row = &(d->rows[i]);
		row->numEntries = 0;
		row->capacity = ROW_INIT_ENTRIES;
		//Allocate space for DictEntry *entries
		if((row->entries=malloc(ROW_INIT_ENTRIES * sizeof(DictEntry)))==NULL){
		dictFree(d);
		return NULL;
		}
	}
	
	return d;
}



