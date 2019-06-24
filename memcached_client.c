#include "memcached_client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define HOST "127.0.0.1"
#define PORT "11211"


// #define ADD "add"
#define ETHERNET_MTU 1500
#define MSG_MAX_SIZE 128

int sock;
void init_connection(){
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == -1){
    perror("couldn't create socket\n");
    exit(EXIT_FAILURE);

  }
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  

  if (getaddrinfo(HOST, PORT, &hints, &res) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(sock));
    exit(EXIT_FAILURE);
  }
  if (connect(sock, res->ai_addr, res->ai_addrlen) != 0){
    printf("couldn't connect. %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  // don't know if i should be freeing this here.
  freeaddrinfo(res);
}

void close_connection(){
  close(sock);
}

void send_all(void* buf, uint_size_t size){
  int total_sent = 0;
  int sent = 0;

  while(total_sent < size){
    buf += sent;
    sent = send(sock, buf, size, 0);
    if(sent == -1){
      perror("couldn't send data\n");
      exit(EXIT_FAILURE);
    }
    total_sent += sent;
  }
}

/*
  receives until the amount of bytes are received
  which is specified in the packet itself
  returns the data_length, not total received length.
 */
int recv_all(void* buf){
  char recvd[ETHERNET_MTU];
  memset(recvd, 0, ETHERNET_MTU);
  int recvd_size = recv(sock, recvd, ETHERNET_MTU, 0);

  if(recvd_size <= 0){
    perror("receive error\n");
    exit(EXIT_FAILURE);
  }
  if(strncmp(recvd, "END", 3) == 0){
    printf("not found\n");
    return -1;
  }

  assert(strncmp(recvd, "VALUE", 5) == 0);
  int index = 0;
  for(int i = 0; i < 3; index++){
    if(recvd[index] == ' ')
      i++;
  }
  uint_size_t data_length = atoi(recvd + index);
  while(recvd[index-2] != '\r' && recvd[index-1] != '\n' && index < recvd_size)
    index++;
  while(strncmp(recvd + index + data_length + 2, "END", 3) != 0){
    recvd_size += recv(sock, recvd + recvd_size, ETHERNET_MTU, 0);
  }
  memcpy(buf, recvd + index, data_length);
  return data_length;
}

void send_block(sector_t sector, void* buf){
  add_entry(sector, buf, SECTOR_SIZE);
}

void recv_block(sector_t sector, void* buf){
  get_entry(sector, buf, SECTOR_SIZE);
}

int add_entry(sector_t sector, void* buf, uint_size_t size){
  char add_statement[MSG_MAX_SIZE];
  memset(add_statement, 0, MSG_MAX_SIZE);

  int stat_len = sprintf(add_statement, "add %u 0 0 %u\r\n", sector, size);
  send_all(add_statement, stat_len);

  char to_send[size + 2];
  memcpy(to_send, buf, size + 2);
  to_send[size] = '\r';
  to_send[size + 1] = '\n';

  send_all(to_send, size + 2);
  char recv_buf [MSG_MAX_SIZE];
  memset(recv_buf, 0, MSG_MAX_SIZE);
  int recv_length = recv(sock, recv_buf, MSG_MAX_SIZE, 0);
  
  if (strcmp(recv_buf, "STORED\r\n") == 0){
    return 0;
  } else {
    printf("%s", recv_buf);
    return -1;
  }
}

int flush_all(){
  char* flush_statement = "flush_all\r\n";
  send_all(flush_statement, strlen(flush_statement));
  char ok[MSG_MAX_SIZE];
  memset(ok, 0, MSG_MAX_SIZE);
  recv(sock, ok, MSG_MAX_SIZE, 0);
  if (strcmp(ok, "OK\r\n") == 0){
    return 0;
  } else {
    printf("%s", ok);
    return -1;
  }
}

int get_entry(sector_t sector, char* buf, uint_size_t size){
  char get_statement[MSG_MAX_SIZE];
  memset(get_statement, 0, MSG_MAX_SIZE);
  int stat_len = sprintf(get_statement, "get %u\r\n", sector);
  // send_block()
  send_all(get_statement, stat_len);
  recv_all(buf);
}



// int main(){

//   init_connection();
//   char* test = "hello";
//   add_entry(0, test, strlen(test));
//   char* buf = malloc(ETHERNET_MTU);
//   get_entry(0, buf, ETHERNET_MTU);
//   printf("%s\n", buf);
//   // printf)
//   flush_all();
//   return 0;
// }