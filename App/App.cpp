/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <unistd.h>
#define MAX_PATH FILENAME_MAX

#include "App.h"
#include "Enclave_u.h"
#include "sgx_urts.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char *msg;
  const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {SGX_ERROR_UNEXPECTED, "Unexpected error occurred.", NULL},
    {SGX_ERROR_INVALID_PARAMETER, "Invalid parameter.", NULL},
    {SGX_ERROR_OUT_OF_MEMORY, "Out of memory.", NULL},
    {SGX_ERROR_ENCLAVE_LOST, "Power transition occurred.",
     "Please refer to the sample \"PowerTransition\" for details."},
    {SGX_ERROR_INVALID_ENCLAVE, "Invalid enclave image.", NULL},
    {SGX_ERROR_INVALID_ENCLAVE_ID, "Invalid enclave identification.", NULL},
    {SGX_ERROR_INVALID_SIGNATURE, "Invalid enclave signature.", NULL},
    {SGX_ERROR_OUT_OF_EPC, "Out of EPC memory.", NULL},
    {SGX_ERROR_NO_DEVICE, "Invalid SGX device.",
     "Please make sure SGX module is enabled in the BIOS, and install SGX "
     "driver afterwards."},
    {SGX_ERROR_MEMORY_MAP_CONFLICT, "Memory map conflicted.", NULL},
    {SGX_ERROR_INVALID_METADATA, "Invalid enclave metadata.", NULL},
    {SGX_ERROR_DEVICE_BUSY, "SGX device was busy.", NULL},
    {SGX_ERROR_INVALID_VERSION, "Enclave version was invalid.", NULL},
    {SGX_ERROR_INVALID_ATTRIBUTE, "Enclave was not authorized.", NULL},
    {SGX_ERROR_ENCLAVE_FILE_ACCESS, "Can't open enclave file.", NULL},
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
  size_t idx = 0;
  size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

  for (idx = 0; idx < ttl; idx++) {
    if (ret == sgx_errlist[idx].err) {
      if (NULL != sgx_errlist[idx].sug)
        printf("Info: %s\n", sgx_errlist[idx].sug);
      printf("Error: %s\n", sgx_errlist[idx].msg);
      break;
    }
  }

  if (idx == ttl)
    printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer "
           "Reference\" for more details.\n",
           ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void) {
  sgx_status_t ret = SGX_ERROR_UNEXPECTED;

  /* Call sgx_create_enclave to initialize an enclave instance */
  /* Debug Support: set 2nd parameter to 1 */
  ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL,
                           &global_eid, NULL);
  if (ret != SGX_SUCCESS) {
    print_error_message(ret);
    return -1;
  }

  return 0;
}

/* OCall functions */
void ocall_print_string(const char *str) {
  /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
  printf("%s", str);
}

int ocall_vecmul(int **vec, int num_of_clients) {

  int res = 0;
  for (int i = 0; i < 100; i++) {
    int tmp = 1;
    for (int j = 0; j < num_of_clients; j++) {
      tmp *= vec[j][i];
    }
    res += tmp;
  }
  return res;
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
  (void)(argc);
  (void)(argv);

  /* Some parameter */
  const char *ip = "127.0.0.1";
  int port = 30000;
  int num_of_clients = 2;

  /* Initialize the enclave */
  if (initialize_enclave() < 0) {
    printf("Enter a character before exit ...\n");
    getchar();
    return -1;
  }
  printf("Info: Successfully initialize enclave.\n");

  /* Create socket */
  int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  /* Bind IP and port */
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);
  bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));

  printf("Ready to accept data...\n");
  /* Listen for the client */
  listen(server_socket, 20);

  struct sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);
  char buffer[400] = {0};

  int **vec_arr = (int **)malloc(num_of_clients * sizeof(int *));

  int client_sockets[num_of_clients];

  for (int i = 0; i < num_of_clients; i++) {
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                               &client_addr_size);
    printf("Receive Client %d's data.\n", i);
    int strlen = recv(client_socket, buffer, 400, 0);

    int *vec = (int *)malloc(100 * sizeof(int));
    memcpy(vec, buffer, 400);

    vec_arr[i] = vec;

    client_sockets[i] = client_socket;

    /* Reset buffer */
    memset(buffer, 0, 400);
  }
  int res;
  ecall_vecmul(global_eid, &res, vec_arr, num_of_clients);

  /* Send the result back to all clients */
  for (int i = 0; i < num_of_clients; i++) {
    send(client_sockets[i], (char *)&res, 4, 0);
    close(client_sockets[i]);
  }

  /* Close socket */
  close(server_socket);

  /* Destroy the enclave */
  sgx_destroy_enclave(global_eid);

  printf("Info: Successfully destroy enclave.\n");

  return 0;
}
