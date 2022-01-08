#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>

#define BACKLOG 5
#define PORT 5000
#define BUFFER_SIZE 1024
#define DELIMITER " "

void *handle_client(void *socket_fd);

int main(int argc, char *argv[]) {
  // Initialize variables
  int master_socket_fd, new_socket_fd;
  struct sockaddr_in server_address, client_address;
  socklen_t client_address_length;
  pthread_t pthread;
  pthread_attr_t pthread_attributes;

  // Initialize server address configurations
  memset(&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);

  // Create the master socket and bind it to the server address
  master_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  bind(master_socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));

  // Listen for incoming connections.
  if (listen(master_socket_fd, BACKLOG) < 0) {
    perror("listen");
    exit(1);
  }

  // Initialize the pthread attributes.
  pthread_attr_init(&pthread_attributes);
  pthread_attr_setdetachstate(&pthread_attributes, PTHREAD_CREATE_DETACHED);

  // Accept connections and create new threads to handle them.
  while (1) {
    client_address_length = sizeof(client_address);

    // Accept a new connection.
    new_socket_fd = accept(master_socket_fd, (struct sockaddr *)&client_address, &client_address_length);
    printf("\nAccepted new connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    // Create a new thread to handle the client.
    pthread_create(&pthread, &pthread_attributes, handle_client, (void *)&new_socket_fd); // NOLINT
  }

  // Close the master socket.
  close(master_socket_fd);

  return 0;
}

void *handle_client(void *socket_fd) {
  int socket_fd_int = *((int *)socket_fd);

  // Read the client's message and convert it to a number.
  char num_buffer[BUFFER_SIZE];
  read(socket_fd_int, num_buffer, BUFFER_SIZE);
  int num = atoi(num_buffer);
  printf("Client requested top %d processes\n", num);

  // Loop over all files in /proc
  int num_processes = 0;
  DIR *proc_directory;
  struct dirent *proc_entry;
  proc_directory = opendir("/proc");
  // Get the number of entries in /proc
  while ((proc_entry = readdir(proc_directory)) != NULL) num_processes++;
  proc_directory = opendir("/proc");

  int i = 0;
  int cputimes[num_processes];
  char processes[num_processes][BUFFER_SIZE];

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
      num_processes--;
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

    sprintf(processes[i], "%d %d %s", cputime, pid, process_name);
    cputimes[i] = cputime;

    i++;
  }

  // Sort the processes by cputime
  for (int i = 0; i < num_processes; i++) {
    for (int j = 0; j < num_processes - 1; j++) {
      if (cputimes[j] < cputimes[j + 1]) {
        int temp_cputime = cputimes[j];
        char temp_process[BUFFER_SIZE];
        strcpy(temp_process, processes[j]);
        cputimes[j] = cputimes[j + 1];
        strcpy(processes[j], processes[j + 1]);
        cputimes[j + 1] = temp_cputime;
        strcpy(processes[j + 1], temp_process);
      }
    }
  }

  // Write the top <num> processes to a file
  char top_processes_file_path[BUFFER_SIZE];
  sprintf(top_processes_file_path, "top_%d_processes.txt", num);
  FILE *top_processes_file = fopen(top_processes_file_path, "w");
  for (int i = 0; i < num; i++) fprintf(top_processes_file, "%s\n", processes[i]);
  int file_size = ftell(top_processes_file);
  fclose(top_processes_file);

  // Send the top processes file to the client
  printf("Sending top %d processes' file to client...\n", num);
  char file_size_buffer[BUFFER_SIZE];
  sprintf(file_size_buffer, "%d", file_size);
  write(socket_fd_int, file_size_buffer, BUFFER_SIZE);
  int top_processes_file_fd = open(top_processes_file_path, O_RDONLY);
  sendfile(socket_fd_int, top_processes_file_fd, NULL, file_size);
  close(top_processes_file_fd);

  // Get top process from client
  printf("Receiving top process from client...\n");
  char top_process[BUFFER_SIZE];
  read(socket_fd_int, top_process, sizeof(top_process));

  // Get cputime, pid, and process name from top process
  char *save_ptr;
  int top_cputime = atoi(strtok_r(top_process, DELIMITER, &save_ptr));
  int top_pid = atoi(strtok_r(NULL, DELIMITER, &save_ptr));
  char *top_process_name = strtok_r(NULL, DELIMITER, &save_ptr);

  // Print the top process
  printf("\nTop process: \nProcess name: %sPID: %d\nCPU Time: %d\n\n\n", top_process_name, top_pid, top_cputime);

  close(socket_fd_int);
  return;
}
