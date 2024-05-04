#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "user.h"
#include "option.h"
#include "readline.h"

// Variables to be cleaned up by signal handler.
int sock_fd;
userlist_t *users;

void handle_connection(int, userlist_t *);

// Handler for sigint
void cleanup(int sig) {
   close(sock_fd);
   free(users->list);
   free(users);
   printf("SIGINT detected, stopping the server...\n");
   exit(0);
}

int main(int argc, char *argv[]) {
   char usage[] = "USAGE:\nserver <PORT> <FILEPATH>";
   int port, client_fd;
   char *filepath;
   struct sockaddr_in server_address, client_address;
   socklen_t addr_len;
   struct sigaction interrupt_action;

   if (argc != 3) {
      fprintf(stderr, "Error not enough argments specified.\n%s\n", usage);
      return EXIT_FAILURE;
   }

   // Block any subsequent sigint signals when handler sees the first signal.
   sigemptyset(&interrupt_action.sa_mask);
   sigaddset(&interrupt_action.sa_mask, SIGINT);

   interrupt_action.sa_flags = 0;
   interrupt_action.sa_handler = cleanup;
   sigaction(SIGINT, &interrupt_action, NULL);

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
   char payload[132];
   char *response;
   char *data;
   bool exists;
   int response_length;
   volatile char *to_write;

   static char affirmative[] =
       "Unfortunately your credentials were in the breached dataset.";

   static char negative[] =
       "Your credentials were not found in the breached dataset.";

   for (;;) {
      choice = EXIT;

      if (!(data = read_to_buf(socket_fd, 500))) {
         fprintf(stderr, "error reading the string to a buffer.\n");
         break;
      }

      sscanf(data, "%d|%131s", &choice, payload);
      printf("%s", payload);

      if (choice == EXIT) {
         free(data);
         break;
      }

      exists = check_exists(payload, users, choice);
      to_write = exists ? affirmative : negative;
      response_length = exists ? sizeof(affirmative) : sizeof(negative);

      // Construct a new response string.
      response = (char *)malloc(sizeof(char) * response_length);
      sprintf(response, "%s", to_write);

      if (write(socket_fd, response, response_length) < 0) {
         perror("unable to send response");
         break;
      }

      free(response);
      free(data);
   }
}
