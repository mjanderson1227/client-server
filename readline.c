#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INITIAL_SIZE 10

char *readline() {
   char *buff, *line;
   char currentChar;
   int size = 0, buffsize = INITIAL_SIZE;

   // Validate malloc calls.
   buff = (char *)malloc(sizeof(char) * buffsize);
   if (!buff) {
      fprintf(stderr, "Failed to allocate memory for the initial buffer.\n");
      free(buff);
      return NULL;
   }

   // Grab a character and put it into the buffer.
   for (;;) {
      currentChar = getchar();

      // Check for EOF
      if (currentChar == EOF) {
         fprintf(stderr, "Found end of file.\n");
         free(buff);
         return NULL;
      }

      // Terminate if a newline is found.
      if (currentChar == '\n')
         break;

      // If the size exceeds the buffer then double the size of the buffer
      if (size >= buffsize) {
         buffsize *= 2;
         buff = (char *)realloc(buff, sizeof(char) * buffsize);
         if (!buff) {
            fprintf(stderr, "Failed to resize the memory buffer.\n");
            free(buff);
            return NULL;
         }
      }

      buff[size] = currentChar;
      size++;
   }

   // Malloc the size of the return string.
   line = malloc(sizeof(char) * size + 1);
   if (!line) {
      fprintf(stderr, "Failed to acquire memory for return string.\n");
      free(buff);
      free(line);
      return NULL;
   }

   // Copy the buffer over to the line.
   if (!memcpy(line, buff, size)) {
      fprintf(stderr, "Failed to copy bytes over to return string.\n");
      free(buff);
      free(line);
      return NULL;
   }

   // Turn the char buffer into a string.
   line[size] = '\0';

   free(buff);
   return line;
}

char *read_to_buf(int socket, int chunk_size) {
   char *buf, *to_return;
   int n_read, mem_size = chunk_size, chunk_limit = chunk_size, read_size;

   buf = (char *)malloc(sizeof(char) * mem_size);
   if (!buf) {
      fprintf(stderr, "Error out of memory.");
      exit(1);
   }

   while ((n_read = read(socket, &buf[mem_size - chunk_limit], chunk_limit)) >=
          chunk_limit) {
      mem_size += chunk_limit;
      buf = realloc(buf, mem_size);
      if (!buf) {
         fprintf(stderr, "Error out of memory.");
         exit(1);
      }
   }

   if (n_read < 0) {
      perror("socket read failed");
      return NULL;
   }

   read_size = mem_size + n_read;

   to_return = (char *)malloc(sizeof(char) * (read_size + 1));
   if (!to_return) {
      fprintf(stderr, "Error out of memory.");
      exit(1);
   }

   strncpy(to_return, buf, read_size);
   to_return[read_size] = '\0';

   free(buf);

   return to_return;
}
