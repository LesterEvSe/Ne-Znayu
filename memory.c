#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void *reallocate(VM *vm, void *pointer, const size_t old_size, const size_t new_size) {
  
  // TODO Ahem? I think this is bad idea, to sub two unsigned values
  // I don't know, why it's work correctly .__.
  vm->bytes_allocated += (new_size - old_size);
  // printf("new size = %lu\nold size = %lu\nres = %lu\n", new_size, old_size, new_size - old_size);
  // printf("vm bytes = %lu\n\n", vm.bytes_allocated);

  if (new_size > old_size) {
// Bad for performance, but good for finding bugs
#ifdef DEBUG_STRESS_GC
    collect_garbage(vm);
#endif

    if (vm->bytes_allocated > vm->next_gc) {
      collect_garbage(vm);
    }
  }
  
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }

  void *result = realloc(pointer, new_size);
  if (result == NULL) exit(1);
  return result;
}

void mark_object(VM *vm, Obj *object) {
  if (object == NULL) return;
  if (object->is_marked) return;  // Because can have cycle of gray objects

#ifdef DEBUG_LOG_GC
  printf("%p mark ", (void*)object);
  print_value(OBJ_VAL(object));
  printf("\n");
#endif

  // Tricolor abstraction. We need it for multithreading approach in future...
  // If white, is_marked == false, if black, then true, if gray, then true and add to worklist
  object->is_marked = true;

  // Can be any other data structure instead of gray stack.
  // Use this, because it easy to implement
  if (vm->gray_capacity < vm->gray_count + 1) {
    vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
    vm->gray_stack = (Obj**)realloc(vm->gray_stack, sizeof(Obj*) * vm->gray_capacity);
    
    if (vm->gray_stack == NULL) exit(1);
  }
  
  vm->gray_stack[vm->gray_count++] = object;
}

void mark_value(VM *vm, Value value) {
  if (IS_OBJ(value)) mark_object(vm, AS_OBJ(value));
}

void mark_table(VM *vm, Table *table) {
  for (int i = 0; i < table->capacity; ++i) {
    Entry *entry = &table->entries[i];
    mark_object(vm, (Obj*)entry->key);
    mark_value(vm, entry->value);
  }
}

static void mark_array(VM *vm, ValueArray *array) {
  for (int i = 0; i < array->length; ++i) {
    mark_value(vm, array->values[i]);
  }
}

// Make black
// More cases with more types
static void blacken_object(VM *vm, Obj *object) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", (void*)object);
  print_value(OBJ_VAL(object));
  printf("\n");
#endif

  switch (object->type) {
    case OBJ_ACTOR: {
      ObjActor *actor = (ObjActor*)object;
      mark_object(vm, (Obj*)actor->name);
      mark_table(vm, &actor->messages);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure *closure = (ObjClosure*)object;
      mark_object(vm, (Obj*)closure->function);
      
      for (int i = 0; i < closure->upvalue_count; ++i) {
        mark_object(vm, (Obj*)closure->upvalues[i]);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction *function = (ObjFunction*)object;
      mark_object(vm, (Obj*)function->name);
      mark_array(vm, &function->chunk.constants);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance *instance = (ObjInstance*)object;
      mark_object(vm, (Obj*)instance->actor);
      mark_table(vm, &instance->fields);
      break;
    }
    case OBJ_UPVALUE:
      mark_value(vm, ((ObjUpvalue*)object)->closed);
      break;
    
    // Because they don't need to be processed
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

static void free_object(VM *vm, Obj *object) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", (void*)object, object->type);
#endif

  switch (object->type) {
    case OBJ_ACTOR: {
      ObjActor *actor = (ObjActor*)object;
      free_table(&actor->messages);
      FREE(vm, ObjActor, object);
      break;
    }
    case OBJ_CLOSURE: {
      const ObjClosure *closure = (ObjClosure*)object;
      // TODO need to uncomment, but there is a bug...
      FREE_ARRAY(vm, ObjUpvalue*, closure->upvalues, closure->upvalue_count);
      FREE(vm, ObjClosure, object);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction *function = (ObjFunction*)object;
      free_chunk(&function->chunk);
      FREE(vm, ObjFunction, object);
      break;
    }
    case OBJ_INSTANCE: {
      ObjInstance *instance = (ObjInstance*)object;
      free_table(&instance->fields);
      FREE(vm, ObjInstance, object);
      break;
    }
    case OBJ_NATIVE:
      FREE(vm, ObjNative, object);
      break;
    case OBJ_STRING: {
      ObjString *string = (ObjString*)object;
      reallocate(vm, string, sizeof(ObjString) +  string->length + 1, 0);
      break;
    }
    case OBJ_UPVALUE:
      FREE(vm, ObjUpvalue, object);
      break;
  }
}

static void mark_roots(VM *vm) {
  for (Value *slot = vm->stack; slot < vm->stack_top; ++slot) {
    mark_value(vm, *slot);
  }

  for (int i = 0; i < vm->frame_count; ++i) {
    mark_object(vm, (Obj*)vm->frames[i].closure);
  }

  for (ObjUpvalue *upvalue = vm->open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
    mark_object(vm, (Obj*)upvalue);
  }

  if (vm->type == MAIN) {
    mark_globals(&main_vm.globals);
    mark_object(vm, (Obj*)main_vm.init_string);
  }
  mark_compiler_roots();
}

static void trace_references(VM *vm) {
  while (vm->gray_count > 0) {
    Obj *object = vm->gray_stack[--vm->gray_count];
    blacken_object(vm, object);
  }
}

// Common "delete node from singly linked list" algorithm
static void sweep(VM *vm) {
  Obj *previous = NULL;
  Obj *object = vm->objects;

  while (object != NULL) {
    if (object->is_marked) {
      object->is_marked = false;
      previous = object;
      object = object->next;
      continue;
    }

    Obj *unreached = object;
    object = object->next;
    if (previous != NULL) {
      previous->next = object;
    } else {
      vm->objects = object;
    }

    free_object(vm, unreached);
  }
}

void collect_garbage(VM *vm) {
#ifdef DEBUG_LOG_GC
  printf("-- gc begin\n");
  const size_t before = vm.bytes_allocated;
#endif

  mark_roots(vm);
  trace_references(vm);
  table_remove_white(vm, &vm->strings);
  sweep(vm);

  vm->next_gc = vm->bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- gc end\n");
  printf("    collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

void free_objects(VM *vm) {
  Obj *object = vm->objects;
  while (object != NULL) {
    Obj *next = object->next;
    free_object(vm, object);
    object = next;
  }

  free(vm->gray_stack);
}