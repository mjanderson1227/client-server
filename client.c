#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include "option.h"
#include "sha256_lib.h"
#include "readline.h"

char *hash_string(char *str) {
   SHA256_CTX ctx;
   uint8_t hash[SHA256_DIGEST_SIZE];
   char *toreturn;
   int i;

   toreturn = (char *)malloc(sizeof(char) * SHA256_DIGEST_SIZE * 2);

   sha256_init(&ctx);
   sha256_update(&ctx, (uint8_t *)str, strlen(str));
   sha256_final(&ctx, hash);

   // Encode the string as hex.
   for (i = 0; i < SHA256_DIGEST_SIZE; i++) {
      sprintf(&toreturn[i * 2], "%02x", hash[i]);
   }

   return toreturn;
}

char *generate_request(SERVER_OPTION opt) {
   char *result;
   char *user_input, *extra_input;
   char *hashed, *hashed_extra;
   char opt_char;

   if (opt == EXIT) {
      return NULL;
   }

   result = (char *)malloc(opt == BOTH ? 150 : 70);

   switch (opt) {
      case USERNAME:
         printf("Enter your username: ");
         user_input = readline();
         hashed = hash_string(user_input);
         sprintf(result, "%d|%s", opt, hashed);

         free(user_input);
         free(hashed);
         break;

      case PASSWORD:
         printf("Enter your password: ");
         user_input = readline();
         hashed = hash_string(user_input);
         sprintf(result, "%d|%s", opt, hashed);

         free(user_input);
         free(hashed);
         break;

      case BOTH:
         printf("Enter your username: ");
         user_input = readline();
         hashed = hash_string(user_input);

         printf("Enter your password: ");
         extra_input = readline();
         hashed_extra = hash_string(extra_input);

         sprintf(result, "%d|%s:%s", opt, hashed, hashed_extra);

         free(user_input);
         free(extra_input);
         free(hashed);
         free(hashed_extra);
         break;

      case EXIT:
         sprintf(result, "%d|%s", opt, "\0");
         break;
   }

   // Print statement for protocol.
   printf("INFO: Request Data Sent: %s\n\n", result);

   return result;
}

void print_menu() {
   printf("1. Check username/email.\n");
   printf("2. Check password.\n");
   printf("3. Check both username/email and password.\n");
   printf("4. End Connection.\n");
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
   char *hostname, *host_entry_name;
   char *user_input;
   char *response;
   struct hostent *host_entry; // Resolve the IP address.

   bool err = false;

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
   addr.sin_port = htons((u_short)port);
   addr.sin_family = AF_INET;
   host_entry = gethostbyname(hostname);
   if (((char *)host_entry) == NULL) {
      fprintf(stderr, "hostname resolve failed\n");
      exit(1);
   }
   // Connect the server to the resolved hostname.
   memcpy(&addr.sin_addr, host_entry->h_addr, host_entry->h_length);

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
      sscanf(user_input, "%d", (int *)&choice);
      printf("choice: %d\n", choice);

      if (!user_input) {
         fprintf(stderr, "Error: unable to parse input line.\n");
         err = true;
         break;
      }

      if (choice > 4 || choice < 0) {
         fprintf(stderr, "Error: Invalid option specified");
         free(user_input);
         continue;
      }

      if (choice == EXIT) {
         free(user_input);
         break;
      }

      // Build the request.
      payload = generate_request(choice);

      // Send the payload.
      if (write(sock_server, payload, strlen(payload) + 1) < 0) {
         perror("unable to send data");
         close(sock_server);
         free(user_input);
         err = true;
         break;
      }

      // Read server response
      if (!(response = read_to_buf(sock_server, 500))) {
         perror("unable to read the server response\n");
         err = true;
         break;
      }

      printf("INFO: Response packet: %s\n", response);

      printf("%s\n", response);
   }

   close(sock_server);

   if (err)
      return EXIT_FAILURE;

   return EXIT_SUCCESS;
}
