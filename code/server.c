#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"

char *bdd_bin = "./bdd";

// On prépare les arguments qui seront envoyés à bdd
// ADD toto poney lundi 3 -> { "./bdd", "ADD", "toto", "poney", lundi", "3", NULL }
char **parse(char *line)
{
  char **res = malloc(7 * sizeof(char *));
  res[0] = bdd_bin;

  char *arg0 = strtok(line, "\n");
  if (strcmp(arg0, "SEE") == 0)
  {
    res[1] = arg0;
    res[2] = NULL;
    return res;
  }

  char *arg1 = strtok(line, " ");
  res[1] = arg1;

  char *arg2 = strtok(NULL, " ");
  res[2] = arg2;
  if (arg2 == NULL)
  {
    arg1[strlen(arg1) - 1] = '\0';
    return res;
  }

  char *arg3 = strtok(NULL, " ");
  res[3] = arg3;
  if (arg3 == NULL)
  {
    arg2[strlen(arg2) - 1] = '\0';
    return res;
  }

  char *arg4 = strtok(NULL, " ");
  res[4] = arg4;
  if (arg4 == NULL)
  {
    arg3[strlen(arg3) - 1] = '\0';
    return res;
  }

  char *arg5 = strtok(NULL, "\n");
  res[5] = arg5;
  res[6] = NULL;
  return res;
}

// Configuration de la socket réseau, retourne le file descriptor de la socket
int configure_socket()
{
  int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc < 0)
    exit_msg("socket", 1);
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(SERVER_IP);
  address.sin_port = htons(SERVER_PORT);
  if (bind(socket_desc, (struct sockaddr *)&address, sizeof(address)) < 0)
    exit_msg("bind", 1);
  if (listen(socket_desc, 3) < 0)
    exit_msg("listen", 1);
  return socket_desc;
}

// Passage des commandes à la base de données par un pipe
// Renvoi des réponses au client par la socket réseau
void process_communication(int socket_desc)
{
  printf("En attente de connexion\n");
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(SERVER_IP);
  address.sin_port = htons(SERVER_PORT);
  socklen_t addrlen = sizeof(address);
  int new_socket = accept(socket_desc, (struct sockaddr *)&address, &addrlen);
  if (new_socket < 0)
    return;
  printf("Connection établie\n");

  char message[REPLY_SIZE];
  read(new_socket, message, REPLY_SIZE);
  char **args = parse(message);

  int pipefd[2];
  pipe(pipefd);
  pid_t pid = fork();
  if (pid == 0)
  {
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    execvp(args[0], args);
  }
  else if (pid < 0)
  {
    perror("fork");
    exit_msg("Commande échouée", 1);
  }
  else
  {
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
      close(pipefd[1]);
      if (strcmp(args[1], "SEE") == 0)
      {
        char replyBuffer[REPLY_SIZE];
        int nbytes = read(pipefd[0], replyBuffer, REPLY_SIZE - 1);
        if (nbytes >= 0)
        {
          replyBuffer[nbytes] = '\0';
        }
        close(pipefd[0]);

        write(new_socket, replyBuffer, REPLY_SIZE);
      }
      else
      {
        char successBuffer[REPLY_SIZE];
        sprintf(successBuffer, "Commande %s effectuée avec succès\n", args[1]);
        write(new_socket, successBuffer, REPLY_SIZE);
      }
    }
    else
    {
      char successBuffer[REPLY_SIZE];
      sprintf(successBuffer, "Commande %s échouée\n", args[1]);
      write(new_socket, successBuffer, REPLY_SIZE);
    }
  }
}

int main(int argc, char **argv)
{
  int socket_desc;

  // Configuration de la socket serveur
  socket_desc = configure_socket();

  // Réception des connections réseaux entrantes

  // Gestion des commandes entrantes dans la nouvelle socket
  while (1)
  {
    process_communication(socket_desc);
  }
}