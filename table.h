#ifndef PL_TABLE_H
#define PL_TABLE_H

#include "common.h"
#include "value.h"

typedef struct {
  ObjString *key;
  Value value;
} Entry;

typedef struct {
  int count;
  int capacity;
  Entry *entries;
} Table;

void init_table(Table *table);
void free_table(Table *table);
bool table_get(const Table *table, const ObjString *key, Value *value);
bool table_set(Table *table, ObjString *key, Value value);
bool table_delete(const Table *table, const ObjString *key);
void table_add_all(const Table *from, Table *to);
ObjString *table_find_string(const Table *table, const char *chars, int length, uint32_t hash);
void mark_table(Table *table);

void table_remove_white(Table *table);

#endif // PL_TABLE_H