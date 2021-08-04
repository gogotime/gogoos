#ifndef __LIB_KERNEL_INTERRUPT_H
#define __LIB_KERNEL_INTERRUPT_H

void idtInit();

enum IntrStatus {
    INTR_OFF,
    INTR_ON
};

void enableIntr();

void disableIntr();

void setIntrStatus(enum IntrStatus status);

enum IntrStatus getIntrStatus();

#endif