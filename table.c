#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void init_table(Table *table) {
  table->count = table->capacity = 0;
  table->entries = NULL;
}

void free_table(Table *table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  init_table(table);
}

static Entry *find_entry(Entry *entries, const int capacity, const ObjString *key) {
  uint32_t index = key->hash % capacity;
  Entry *tombstone = NULL;

  for (;;) {
    Entry *entry = &entries[index];
    if (entry->key == NULL) {
      // Empty entry
      if (IS_NIL(entry->value)) return tombstone != NULL ? tombstone : entry;
      if (tombstone == NULL) tombstone = entry;
    } else if (entry->key == key) { // Can compare two objects, due to string interning
      return entry;
    }
    index = (index + 1) % capacity;
  }
}

bool table_get(const Table *table, const ObjString *key, Value *value) {
  if (table->count == 0) return false;

  const Entry *entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

static void adjust_capacity(Table *table, const int capacity) {
  Entry *entries = ALLOCATE(Entry, capacity);
  table->count = 0;

  for (int i = 0; i < capacity; ++i) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  for (int i = 0; i < table->capacity; ++i) {
    const Entry *entry = &table->entries[i];
    if (entry->key == NULL) continue;

    Entry *dest = find_entry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    ++table->count;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool table_set(Table *table, ObjString *key, const Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    const int capacity = GROW_CAPACITY(table->capacity);
    adjust_capacity(table, capacity);
  }

  Entry *entry = find_entry(table->entries, table->capacity, key);
  const bool is_new_key = entry->key == NULL;
  if (is_new_key && IS_NIL(entry->value)) ++table->count;

  entry->key = key;
  entry->value = value;
  return is_new_key;
}

bool table_delete(const Table *table, const ObjString *key) {
  if (table->count == 0) return false;

  Entry *entry = find_entry(table->entries, table->capacity, key);
  if (entry->key == NULL) return false;

  // Place a tombstone in the entry :D
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}

// Required for inheriting methods
void table_add_all(const Table *from, Table *to) {
  for (int i = 0; i < from->capacity; ++i) {
    const Entry *entry = &from->entries[i];
    if (entry->key != NULL) {
      table_set(to, entry->key, entry->value);
    }
  }
}

ObjString *table_find_string(const Table *table, const char *chars, const int length, const uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash % table->capacity;
  for (;;) {
    const Entry *entry = &table->entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->length == length &&
        entry->key->hash == hash &&
        memcmp(entry->key->chars, chars, length) == 0) {
      return entry->key;
    }
    index = (index + 1) % table->capacity;
  }
}