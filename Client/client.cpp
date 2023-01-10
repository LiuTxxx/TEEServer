#include <arpa/inet.h>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int main() {

  clock_t start, end;
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in server_addr;

  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(30010);
  connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

  /* Construct a vector */
  int vector[100];
  for (int i = 0; i < 100; i++) {
    vector[i] = 11;
  }

  char *buf = (char *)vector;

  start = clock();
  send(sock, buf, 400, 0);

  /* Read from server */
  char buffer[5];

  read(sock, buffer, sizeof(buffer) - 1);
  end = clock();

  int *res = (int *)buffer;
  double duration = (double)(end - start) / CLOCKS_PER_SEC;

  printf("Client receives result %d, cost %f secs\n", *res, duration);
  close(sock);
  return 0;
}
