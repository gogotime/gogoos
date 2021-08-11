#include "list.h"

void listInit(List* list) {
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

void listInsertBefore(ListElem* before, ListElem* elem) {
    IntrStatus status = getIntrStatus();
    disableIntr();

    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;

    setIntrStatus(status);
}

void listPush(List* list, ListElem* elem) {
    listInsertBefore(list->head.next, elem);
}

void listAppend(List* list, ListElem* elem) {
    listInsertBefore(&list->tail, elem);
}

void listRemove(ListElem* elem) {
    IntrStatus status = getIntrStatus();
    disableIntr();

    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    setIntrStatus(status);
}

ListElem* listPop(List* list) {
    ListElem* elem = list->head.next;
    listRemove(elem);
    return elem;
}


bool listElemExist(List* list, ListElem* elem) {
    ListElem* e = list->head.next;
    while (e != &list->tail) {
        if (e == elem) {
            return true;
        }
        e = e->next;
    }
    return false;
}


bool listIsEmpty(List* list) {
    return list->head.next == &list->tail;
}

uint32 listLen(List* list) {
    ListElem* elem = list->head.next;
    uint32 length = 0;
    while (elem != &list->tail) {
        length++;
        elem = elem->next;
    }
    return length;
}

ListElem* listTraversal(List* list, TestElemFunc func, int arg) {
    if (listIsEmpty(list)) {
        return NULL;
    }
    ListElem* elem = list->head.next;
    while (elem != &list->tail) {
        if (func(elem, arg)) {
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

