#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define PORT 5000
#define HOSTNAME "127.0.0.1"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
  int client_socket_fd;
  struct sockaddr_in server_address;

  // Initialize server address configurations
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(HOSTNAME);
  server_address.sin_port = htons(PORT);

  // Create client socket
  if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error opening socket");
    exit(1);
  }

  // Connect to server
  if (connect(client_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
    perror("Error connecting");
    exit(1);
  }

  // Copy argument to buffer
  int num = atoi(argv[1]);
  char buffer[BUFFER_SIZE];
  sprintf(buffer, "%d", num);

  // Send buffer to server
  printf("Requesting top %d processes from server...\n", num);
  write(client_socket_fd, buffer, BUFFER_SIZE);

  // Receive file size from server
  printf("Receiving file size from server...\n");
  char file_size_buffer[BUFFER_SIZE];
  read(client_socket_fd, &file_size_buffer, BUFFER_SIZE);
  int file_size = atoi(file_size_buffer);
  printf("File size: %d\n", file_size);

  // Receive file contents from server
  printf("Receiving file contents from server...\n");
  char file_buffer[file_size];
  read(client_socket_fd, file_buffer, file_size);
  file_buffer[file_size] = '\0';

  // Write the top <num> processes to a file
  char top_processes_file_path[BUFFER_SIZE];
  sprintf(top_processes_file_path, "recevied_top_%d_processes.txt", num);
  FILE *top_processes_file = fopen(top_processes_file_path, "w");
  fprintf(top_processes_file, "%s", file_buffer);
  fclose(top_processes_file);

  // Send highest line to server
  printf("Sending top process to client...\n");
  char *highest_line = strtok(file_buffer, "\n");
  write(client_socket_fd, highest_line, strlen(highest_line));

  sleep(10);
  printf("Closing connection...\n");
  close(client_socket_fd);

  return 0;
}
