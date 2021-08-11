# ifndef __LIB_STRUCTURE_LIST_H
# define __LIB_STRUCTURE_LIST_H

#include "../stdint.h"
#include "../kernel/interrupt.h"

#define offset(structType, member) (uint32)(&((structType*)0)->member)
#define elemToEntry(structType, structMemberName, elemPtr) \
        (structType*)((uint32) elemPtr - offset(structType,structMemberName))

typedef struct listElem {
   struct listElem * prev;
   struct listElem * next;
} ListElem;

typedef struct {
    ListElem head;
    ListElem tail;
} List;

typedef bool (TestElemFunc)(ListElem * elem, int arg);

void listInit(List* list);

void listInsertBefore(ListElem * before, ListElem * elem);

void listPush(List* list, ListElem* elem);

void listAppend(List* list, ListElem* elem);

void listRemove(ListElem * elem);

ListElem* listPop(List* list);

bool listIsEmpty(List* list);

uint32 listLen(List* list);

ListElem* listTraversal(List* list, TestElemFunc func, int arg);

bool listElemExist(List* list, ListElem* elem);

# endif