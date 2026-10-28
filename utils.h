#ifndef UTILS_H
#define UTILS_H

#include "common.h"
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Input exhaustion navigates to the active menu instead of leaving nested loops stuck. */
void railway_set_navigation_target(jmp_buf *target);
void initializeSystem(void);

void cleanupSystem(void);


int getIntegerInput(const char *message);

void getStringInput(const char *message,
                    char *buffer,
                    int size);
float getFloatInput(
    const char *message);
int isValidJourneyDate(const char *date);

#ifdef __cplusplus
}
#endif
    
#endif
