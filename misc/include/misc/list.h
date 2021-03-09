#ifndef _LIST_H_
#define _LIST_H_

#include <stddef.h>

typedef struct _LIST{
    struct _LIST	*next;
    struct _LIST	*prev;
} list_t;

#define list_entry(ptr, type, member)		\
    (type*)((char*)(ptr) - offsetof(type, member))

#define STATIC_LIST_INITIALIZER(list)		{ &(list), &(list) }

static inline list_t* list_first_elem(list_t *list){
    return list->next;
}

static inline list_t* list_last_elem(list_t *list){
    return list->prev;
}

static inline void list_add_front(list_t *list, list_t *new_elem){
    // Generate SIGSEGV for debug
    if ((new_elem->next != NULL) || (new_elem->prev != NULL)) __builtin_trap();

    new_elem->next = list->next;
    new_elem->prev = list;
    list->next->prev = new_elem;
    list->next = new_elem;
}

static inline void list_add_back(list_t *list, list_t *new_elem){
    // Generate SIGSEGV for debug
    if ((new_elem->next != NULL) || (new_elem->prev != NULL)) __builtin_trap();

    new_elem->next = list;
    new_elem->prev = list->prev;
    list->prev->next = new_elem;
    list->prev = new_elem;
}

static inline void list_insert_after(list_t *list, list_t *elem, list_t *new_elem){
    (void)list;

    // Generate SIGSEGV for debug
    if ((new_elem->next != NULL) || (new_elem->prev != NULL)) __builtin_trap();

    new_elem->next = elem->next;
    new_elem->prev = elem;
    elem->next->prev = new_elem;
    elem->next = new_elem;
}

static inline void list_insert_before(list_t *list, list_t *elem, list_t *new_elem){
    (void)list;

    // Generate SIGSEGV for debug
    if ((new_elem->next != NULL) || (new_elem->prev != NULL)) __builtin_trap();

    new_elem->next = elem;
    new_elem->prev = elem->prev;
    elem->prev->next = new_elem;
    elem->prev = new_elem;
}

static inline void list_replace_elem(list_t *list, list_t *elem, list_t *new_elem){
    (void)list;
    new_elem->next = elem->next;
    new_elem->prev = elem->prev;
    elem->next->prev = new_elem;
    elem->prev->next = new_elem;
    elem->next = elem->prev = NULL;
}

static inline void list_remove_elem(list_t *list, list_t *elem){
    (void)list;
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    elem->next = elem->prev = NULL;
}

static inline int list_is_empty(list_t *list){
    return (list == list->next);
}

static inline int list_is_valid_elem(list_t *list, list_t *elem){
    return (elem != list);
}

static inline void list_init(list_t *list){
    list->next = list->prev = list;
}

#endif	/* _LIST_H_ */
