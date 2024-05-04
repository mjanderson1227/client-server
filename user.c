#include <unistd.h>
#include "user.h"
#include <stdlib.h>
#include "option.h"
#include <stdio.h>
#include <string.h>

#define LINE_SIZE 130

userlist_t *read_users(char *filename) {
   int i;
   int memsize = 10;
   FILE *file;
   user_t *users;
   userlist_t *userlist;
   int userlist_size = 0, userlist_mem_size = 10;
   char buf[LINE_SIZE];

   file = fopen(filename, "rb");
   if (!file) {
      fprintf(stderr, "Error: File does not exist.\n");
      return NULL;
   }

   userlist = (userlist_t *)malloc(sizeof(userlist_t));
   if (!userlist) {
      fprintf(stderr,
              "Error: failed allocating memory for the userlist struct.\n");
      return NULL;
   }

   users = (user_t *)malloc(sizeof(user_t) * userlist_mem_size);
   if (!users) {
      fprintf(stderr, "Error: failed allocating memory for the user array.\n");
      return NULL;
   }

   i = 0;
   while (fgets(buf, LINE_SIZE, file)) {
      if (userlist_size + 1 > userlist_mem_size) {
         userlist_mem_size *= 2;
         users = realloc(users, userlist_mem_size);
         if (!users) {
            fprintf(stderr,
                    "Error: failed allocating memory for the user array.\n");
            return NULL;
         }
      }

      sscanf(buf, "%[^:]:%s", users[i].username, users[i].password);
      i++;
   }

   fclose(file);

   userlist->list = users;
   userlist->size = i;

   return userlist;
}

bool check_exists(char *payload, userlist_t *users, SERVER_OPTION opt) {
   int i;
   char joined[150];

   for (i = 0; i < users->size; i++) {
      switch (opt) {
         case USERNAME:
            if (strncmp(users->list[i].username, payload, 64) == 0) {
               return true;
            }
            break;
         case PASSWORD:
            if (strncmp(users->list[i].password, payload, 64) == 0) {
               return true;
            }
            break;
         case BOTH:
            sprintf(joined, "%s:%s", users->list[i].username,
                    users->list[i].password);
            if (strncmp(joined, payload, 130) == 0) {
               return true;
            }
            break;
            // This case should never be encountered.
         case EXIT:
            fprintf(stderr, "Error: passing exit as an option is undefined for "
                            "check_exists()");
            exit(1);
            break;
      }
   }

   return false;
}
