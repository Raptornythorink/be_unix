#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "utils.h"

void send_message(struct sockaddr_in serv_addr, char *message)
{
  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc < 0)
    exit_msg("socket", 1);
  if (connect(socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    exit_msg("connect", 1);
  send(socket_desc, message, strlen(message), 0);
  char reply[REPLY_SIZE];
  recv(socket_desc, reply, REPLY_SIZE, 0);
  printf("Server reply:\n%s\n", reply);
  shutdown(socket_desc, SHUT_RDWR);
  close(socket_desc);
}

int main(int argc, char **argv)
{
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  serv_addr.sin_port = htons(SERVER_PORT);

  send_message(serv_addr, "ADD Gauvin poney mardi 15");

  send_message(serv_addr, "ADD Gauvin poney lundi 15");

  send_message(serv_addr, "ADD Lancelot réunion lundi 14");

  send_message(serv_addr, "ADD Lancelot réunion lundi 16");

  send_message(serv_addr, "ADD Gauvin entraînement lundi 15");

  send_message(serv_addr, "SEE");

  send_message(serv_addr, "SEE_DAY lundi");

  send_message(serv_addr, "SEE_USER Gauvin");

  send_message(serv_addr, "DEL Gauvin poney lundi 15");

  send_message(serv_addr, "DEL Gauvin poney mardi 15");

  send_message(serv_addr, "DEL Lancelot réunion lundi 16");

  send_message(serv_addr, "DEL Lancelot réunion lundi 14");

  send_message(serv_addr, "SEE");

  return 0;
}
