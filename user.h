#ifndef USER_H
#define USER_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct User
{
    int passengerId;

    char username[100];

    char password[100];

    struct User *prev;
    struct User *next;

} User;

extern User *userHead;

void initializeUserModule(void);

void loadUserData(void);

int saveUserData(void);

int registerUser(void);

int loginUser(void);


#ifdef __cplusplus
}
#endif


#endif
