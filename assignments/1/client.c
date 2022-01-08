#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <dirent.h>

#define PORT 5000
#define HOSTNAME "127.0.0.1"
#define BUFFER_SIZE 1024
#define DELIMITER " "

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

  // Get process with max cpu usage
  int max_cputime = 0;
  char max_cputime_line[BUFFER_SIZE];

  DIR *proc_directory;
  struct dirent *proc_entry;
  proc_directory = opendir("/proc");
  while ((proc_entry = readdir(proc_directory)) != NULL) {
    // Open the /proc/<pid>/stat file
    char stat_file_path[BUFFER_SIZE];
    sprintf(stat_file_path, "/proc/%s/stat", proc_entry->d_name);
    int stat_file_fd = open(stat_file_path, O_RDONLY);

    if (
      // Skip the . and .. directories and any non-process directories
      strcmp(proc_entry->d_name, ".") == 0 ||
      strcmp(proc_entry->d_name, "..") == 0 ||
      // Skip any directories that are not numeric
      !isdigit(proc_entry->d_name[0]) ||
      // This means the entry is either not a directory or not a process
      stat_file_fd < 0
    ) {
      if (stat_file_fd > 0) close(stat_file_fd);
      continue;
    };

    // Read the /proc/<pid>/stat file
    char stat_file_buffer[BUFFER_SIZE];
    if (read(stat_file_fd, stat_file_buffer, sizeof(stat_file_buffer)) < 0) {
      perror("read");
      exit(1);
    }
    close(stat_file_fd);

    // Get the pid, process name, cputime
    char *save_ptr;
    int pid = atoi(strtok_r(stat_file_buffer, DELIMITER, &save_ptr));
    char *process_name = strtok_r(NULL, DELIMITER, &save_ptr);
    for (int i = 0; i < 11; i++) strtok_r(NULL, DELIMITER, &save_ptr);
    int utime = atoi(strtok_r(NULL, DELIMITER, &save_ptr));
    int stime = atoi(strtok_r(NULL, DELIMITER, &save_ptr));
    int cputime = utime + stime;

    // If this process has the highest cputime, save it
    if (cputime > max_cputime) {
      max_cputime = cputime;
      sprintf(max_cputime_line, "%d %d %s\n", cputime, pid, process_name);
    }
  }

  // Send the max cputime line to the server
  printf("Sending max cputime line to server...\n");
  write(client_socket_fd, max_cputime_line, BUFFER_SIZE);

  sleep(10);
  printf("Closing connection...\n");
  close(client_socket_fd);

  return 0;
}
