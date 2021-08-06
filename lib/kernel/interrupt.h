#ifndef __LIB_KERNEL_INTERRUPT_H
#define __LIB_KERNEL_INTERRUPT_H

#include "../stdint.h"

typedef enum  {
    INTR_OFF,
    INTR_ON
}IntrStatus;

void setIntrStatus(IntrStatus status);

IntrStatus getIntrStatus();

void enableIntr();

void disableIntr();

#endif

