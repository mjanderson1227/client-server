#ifndef USER_H
#define USER_H

#include <stdbool.h>
#include <stdint.h>
#include "option.h"

typedef struct user_t {
   char username[65];
   char password[65];
} user_t;

typedef struct userlist_t {
   user_t *list;
   int size;
} userlist_t;

userlist_t *read_users(char *);

bool check_exists(char *, userlist_t *, SERVER_OPTION);

#endif
