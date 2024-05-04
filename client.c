#include <stdlib.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include "readline.h"
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include "option.h"

char *generate_request(SERVER_OPTION opt) {
   char *result;
   char *user_input, *extra_input;

   if (opt == EXIT) {
      return NULL;
   }

   result = (char *)malloc(opt == BOTH ? 131 : 66);

   switch (opt) {
      case USERNAME:
         printf("Enter your username: ");
         user_input = readline();
         sprintf(result, "%d%s", opt, user_input);

         free(user_input);
         break;

      case PASSWORD:
         printf("Enter your password: ");
         user_input = readline();
         sprintf(result, "%d%s", opt, user_input);

         free(user_input);
         break;

      case BOTH:
         printf("Enter your username: ");
         user_input = readline();

         printf("Enter your password: ");
         extra_input = readline();

         // TODO: possible error here.
         sprintf(result, "%d%s:%s", opt, user_input, extra_input);

         free(user_input);
         free(extra_input);
         break;

      case EXIT:
         break;
   }

   return result;
}

void print_menu() {
   printf("1. Check username/email.\n");
   printf("2. Check password.\n");
   printf("3. Check both username/email and password.\n");
   printf("4. Check username/email\n");
   printf("Enter your choice: ");
}

int main(int argc, char *argv[]) {
   struct sockaddr_in addr;
   SERVER_OPTION choice;
   char message[65];
   char *payload;
   char *response_data;
   char usage[] = "USAGE: client <HOSTNAME> <PORT>";
   int port;
   int sock_server;
   int response_length;
   socklen_t alen;
   char *hostname;
   char *user_input;
   char *response;

   if (argc != 3) {
      fprintf(
          stderr,
          "An invalid number of command line arguments were supplied.\n%s\n",
          usage);
      return EXIT_FAILURE;
   }

   hostname = argv[1];
   port = atoi(argv[2]);

   memset(&addr, 0, sizeof(addr));
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons((u_short)port);
   addr.sin_family = AF_INET;

   sock_server = socket(PF_INET, SOCK_STREAM, 0);
   if (sock_server < 0) {
      perror("socket create failed");
      return EXIT_FAILURE;
   }

   alen = sizeof(addr);
   if (connect(sock_server, (struct sockaddr *)&addr, alen) < 0) {
      perror("socket connect failed.");
      return EXIT_FAILURE;
   }
   printf("Socket connection success.\n");

   for (;;) {
      print_menu();
      user_input = readline();
      sscanf(user_input, "%d", &choice);
      printf("choice: %d\n", choice);

      if (!user_input || choice > 4 || choice < 0) {
         close(sock_server);
         free(user_input);
         fprintf(stderr, "Error: unable to parse input line.\n");
         return EXIT_FAILURE;
      }

      if (choice > 4 || choice < 0) {
         fprintf(stderr, "Error: Invalid option specified");
         free(user_input);
         continue;
      }

      // Build the request.
      payload = generate_request(choice);

      // Send the payload.
      if (write(sock_server, payload, strlen(payload) + 1) < 0) {
         perror("unable to send data");
         close(sock_server);
         free(user_input);
         return EXIT_FAILURE;
      }

      // Read server response
      if (!(response = read_to_buf(sock_server, 500))) {
         perror("unable to read the server response\n");
         return EXIT_FAILURE;
      }

      printf("%s\n", response);
   }

   close(sock_server);

   return EXIT_SUCCESS;
}
