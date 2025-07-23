#include "avm.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "avm_instruction.h"



#define AVM_TABLE_HASHSIZE 211

typedef struct avm_table_bucket {
    avm_memcell key;
    avm_memcell value;
    struct avm_table_bucket* next;
} avm_table_bucket;

typedef struct avm_table {
    unsigned refCounter;
    avm_table_bucket* numIndexed[AVM_TABLE_HASHSIZE];
    avm_table_bucket* strIndexed[AVM_TABLE_HASHSIZE];
    unsigned total;
} avm_table;


static unsigned hash_num(double num) {
    return ((unsigned)num) % AVM_TABLE_HASHSIZE;
}
static unsigned hash_str(const char* str) {
    unsigned hash = 0;
    while (*str)
        hash = hash * 31 + *str++;
    return hash % AVM_TABLE_HASHSIZE;
}

/* Table creation   */
avm_table* avm_tablenew(void) {
    avm_table* t = (avm_table*)malloc(sizeof(avm_table));
    assert(t);
    t->refCounter = 0;
    memset(t->numIndexed, 0, sizeof(t->numIndexed));
    memset(t->strIndexed, 0, sizeof(t->strIndexed));
    t->total = 0;
    return t;
}

/* Reference counting   */
void avm_tableincrefcounter(avm_table* t) {
    ++t->refCounter;
}


/* Decrements the reference counter of the table and frees it if it reaches zero. */
void avm_tabledecrefcounter(avm_table* t) {
    assert(t->refCounter > 0);
    if (--t->refCounter == 0) {
        for (unsigned i = 0; i < AVM_TABLE_HASHSIZE; ++i) {
            avm_table_bucket* b;
            while ((b = t->numIndexed[i])) {
                t->numIndexed[i] = b->next;
                avm_memcellclear(&b->key);
                avm_memcellclear(&b->value);
                free(b);
            }
            while ((b = t->strIndexed[i])) {
                t->strIndexed[i] = b->next;
                avm_memcellclear(&b->key);
                avm_memcellclear(&b->value);
                free(b);
            }
        }
        free(t);
    }
}

/* Table get element    */
avm_memcell* avm_tablegetelem(avm_table* t, avm_memcell* key) {
   //  printf("DEBUG: avm_tablegetelem: key type=%d, num=%f\n", key->type, key->data.numVal);
    avm_table_bucket* b = NULL;
    if (key->type == number_m) {
        unsigned idx = hash_num(key->data.numVal);
        b = t->numIndexed[idx];
        while (b) {
            if (b->key.type == number_m && b->key.data.numVal == key->data.numVal)
                return &b->value;
            b = b->next;
        }
    } else if (key->type == string_m) {
        unsigned idx = hash_str(key->data.strVal);
        b = t->strIndexed[idx];
        while (b) {
            if (b->key.type == string_m && strcmp(b->key.data.strVal, key->data.strVal) == 0)
                return &b->value;
            b = b->next;
        }
    }
    return NULL;
}

/* Table set element    */
void avm_tablesetelem(avm_table* t, avm_memcell* key, avm_memcell* value) {
   // printf("DEBUG: avm_tablesetelem (start): key type=%d, num=%f, value type=%d, num=%f\n",
      //  key->type,
      //  key->type == number_m ? key->data.numVal : 0.0,
      //  value->type,
      //  value->type == number_m ? value->data.numVal : 0.0);
    avm_table_bucket **bucket_list = NULL, *b = NULL, *prev = NULL;
    unsigned idx = 0;
    if (key->type == number_m) {
        idx = hash_num(key->data.numVal);
        bucket_list = &t->numIndexed[idx];
    } else if (key->type == string_m) {
        idx = hash_str(key->data.strVal);
        bucket_list = &t->strIndexed[idx];
    } else {
        return;
    }
    b = *bucket_list;
    while (b) {
       //  printf("DEBUG: avm_tablesetelem: bucket key type=%d\n", b->key.type);
        if ((key->type == number_m && b->key.type == number_m && b->key.data.numVal == key->data.numVal) ||
            (key->type == string_m && b->key.type == string_m && strcmp(b->key.data.strVal, key->data.strVal) == 0)) {
            // If value is nil, remove the entry
            if (value->type == nil_m) {
                if (prev)
                    prev->next = b->next;
                else
                    *bucket_list = b->next;
                avm_memcellclear(&b->key);
                avm_memcellclear(&b->value);
                free(b);
                t->total--;
            } else {
                avm_memcellclear(&b->value);
                avm_assign(&b->value, value);
            }
            return;
        }
        prev = b;
        b = b->next;
      //  printf("DEBUG: avm_tablesetelem: exited while\n");
    }
    if (value->type != nil_m) {
        avm_table_bucket* newb = (avm_table_bucket*)malloc(sizeof(avm_table_bucket));
        memset(newb, 0, sizeof(avm_table_bucket)); 
      //  printf("DEBUG: avm_tablesetelem: newb=%p, key type=%d, value type=%d\n", (void*)newb, key->type, value->type);
  //  if (key->type == number_m) printf("DEBUG: key num = %f\n", key->data.numVal);
  //  if (value->type == number_m) printf("DEBUG: value num = %f\n", value->data.numVal);
 //   if (key->type == string_m && key->data.strVal) printf("DEBUG: key str = %s\n", key->data.strVal);
  //  if (value->type == string_m && value->data.strVal) printf("DEBUG: value str = %s\n", value->data.strVal);


        avm_assign(&newb->key, key);
        avm_assign(&newb->value, value);
        newb->next = *bucket_list;
        *bucket_list = newb;
        t->total++;
    }
}

/* Table total members  */
unsigned avm_table_totalmembers(avm_table* t) {
    return t->total;
}

/* Table keys (for objectmemberkeys)    */
unsigned avm_table_keys(avm_table* t, avm_memcell* result_array) {
    unsigned count = 0;
    for (unsigned i = 0; i < AVM_TABLE_HASHSIZE; ++i) {
        avm_table_bucket* b = t->numIndexed[i];
        while (b) {
          //  printf("DEBUG: avm_table_keys: key type=%d, num=%f\n", b->key.type, b->key.data.numVal);
            memset(&result_array[count], 0, sizeof(avm_memcell));
            avm_assign(&result_array[count++], &b->key);
          //  printf("DEBUG: avm_table_keys: result_array[%u] type=%d, num=%f\n", count-1, result_array[count-1].type, result_array[count-1].data.numVal);
            b = b->next;
        }
        b = t->strIndexed[i];
        while (b) {
          //  printf("DEBUG: avm_table_keys: key type=%d, num=%f\n", b->key.type, b->key.data.numVal);
            memset(&result_array[count], 0, sizeof(avm_memcell));
            avm_assign(&result_array[count++], &b->key);
          //  printf("DEBUG: avm_table_keys: result_array[%u] type=%d, num=%f\n", count-1, result_array[count-1].type, result_array[count-1].data.numVal);
            b = b->next;
        }
    }
    return count;
}