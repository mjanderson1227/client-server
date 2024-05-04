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
userlist_t *users;
int opened_client_socket = 0;
int sock_fd = 0;

void handle_connection();

// Handler for sigint - Can't get this part to work correctly.
void cleanup(int sig) {
   printf("\nSIGINT detected, stopping the server...\n");

   if (opened_client_socket && close(opened_client_socket) == -1) {
      printf("The connection socket was not being used when it was closed\n");
   }

   if (close(sock_fd) == -1) {
      printf("The welcome socket was not being used when it was closed\n");
   }

   free(users->list);
   free(users);
   exit(0);
}

/*
 * Function Main:
 * ----------------
 *  Main function for the server. Handles the connection and the
 *  response to the client.
 */
int main(int argc, char *argv[]) {
   char usage[] = "USAGE:\nserver <PORT> <FILEPATH>";
   int port, client_fd, sock_opt = 1;
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
      opened_client_socket =
          accept(sock_fd, (struct sockaddr *)&client_address, &addr_len);
      if (opened_client_socket < 0) {
         perror("Socket accept failed.");
         continue;
      }
      handle_connection();
      if (close(opened_client_socket) < 0) {
         perror("close failed");
      }

      printf("Client connection was terminated.\n");
      opened_client_socket = -1;
   }

   free(users->list);
   free(users);
   close(sock_fd);

   return EXIT_SUCCESS;
}

/*
 * Function handle_connection:
 * ----------------
 * Handles the connection between the server and the client.
 * will only return when the client ends the session.
 */
void handle_connection() {
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

   printf("Established connection with client.\n");

   for (;;) {
      choice = EXIT;

      if (!(data = read_to_buf(opened_client_socket, 500))) {
         break;
      }

      sscanf(data, "%d|%131s", (int *)&choice, payload);

      if (choice == EXIT ||
          strlen(payload) < 1) { // If the client writes a 0 payload then the
                                 // connection is over
         free(data);
         break;
      }

      // Print statement for protocol
      printf("INFO: Server data recieved: %s\n", data);

      exists = check_exists(payload, users, choice);
      to_write = exists ? affirmative : negative;
      response_length = exists ? sizeof(affirmative) : sizeof(negative);

      // Construct a new response string.
      response = (char *)malloc(sizeof(char) * response_length);
      sprintf(response, "%s", to_write);
      printf("INFO: Sending response string: %s\n", response);

      if (write(opened_client_socket, response, response_length) < 0) {
         perror("unable to send response");
         break;
      }

      free(response);
      free(data);
   }
}
