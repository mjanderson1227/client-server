#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "user.h"
#include "option.h"
#include "readline.h"

void handle_connection(int fd, userlist_t *data);

int main(int argc, char *argv[]) {
   char usage[] = "USAGE:\nserver <PORT> <FILEPATH>";
   int sock_fd, port, client_fd;
   char *filepath;
   struct sockaddr_in server_address, client_address;
   socklen_t addr_len;
   userlist_t *users;

   if (argc != 3) {
      fprintf(stderr, "Error not enough argments specified.\n%s\n", usage);
      return EXIT_FAILURE;
   }

   port = atoi(argv[1]);
   filepath = argv[2];

   users = read_users(filepath);
   if (!users) {
      fprintf(stderr, "An error occurred reading the file.\n");
      return EXIT_FAILURE;
   }

   sock_fd = socket(PF_INET, SOCK_STREAM, 0);
   if (sock_fd < 0) {
      perror("An error occurred while creating the socket.");
      return EXIT_FAILURE;
   }

   // Variable addr length requires a completely reset struct.
   memset(&server_address, 0, sizeof(struct sockaddr_in));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = INADDR_ANY;
   server_address.sin_port = htons((u_short)port);

   if (bind(sock_fd, (struct sockaddr *)&server_address,
            sizeof(server_address)) < 0) {
      perror("An error occurred while binding the socket.");
      return EXIT_FAILURE;
   }

   // Max connections at a time: 10
   if (listen(sock_fd, 10) < 0) {
      perror("An error occurred while listening to the socket");
      return EXIT_FAILURE;
   }

   for (;;) {
      client_fd =
          accept(sock_fd, (struct sockaddr *)&client_address, &addr_len);
      printf("Successfully accepted a connection.\n");
      if (client_fd < 0) {
         perror("Socket accept failed.");
         continue;
      }
      handle_connection(client_fd, users);
      close(client_fd);
   }

   free(users->list);
   free(users);
   close(sock_fd);

   return EXIT_SUCCESS;
}

void handle_connection(int socket_fd, userlist_t *users) {
   SERVER_OPTION choice;
   char *data, *to_write, buf[131];
   char *response;
   int response_length;

   static char affirmative[] =
       "Unfortunately your password was in the breached dataset.";

   static char negative[] =
       "Your password was not found in the breached dataset.";

   for (;;) {
      data = read_to_buf(socket_fd, 500);

      if (!data || choice == EXIT) {
         break;
      }

      sscanf(data, "%d%130s", &choice, buf);

      to_write = check_exists(data, users, choice) ? affirmative : negative;
      response_length = strlen(to_write);

      // Construct a new response string.
      response = (char *)malloc(sizeof(char) * response_length);
      sprintf(response, "%s", to_write);

      if (write(socket_fd, response, strlen(response)) < 0) {
         perror("unable to send response");
         break;
      }

      free(data);
   }
}
