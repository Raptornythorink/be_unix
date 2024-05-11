#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include "utils.h"

int socket_desc;
sem_t *reader_sem;
sem_t *writer_sem;
sem_t *log_sem;
char *bdd_bin = "./bdd";

void signal_handler(int signal)
{
  if (signal == SIGINT)
  {
    sem_unlink(READER_SEM_NAME);
    sem_unlink(WRITER_SEM_NAME);
    sem_unlink(LOG_SEM_NAME);
    close(socket_desc);
    exit_msg("Server stopped", 0);
  }
}

// On prépare les arguments qui seront envoyés à bdd
// ADD toto poney lundi 3 -> { "./bdd", "ADD", "toto", "poney", lundi", "3", NULL }
char **parse(char *line)
{
  char **res = malloc(7 * sizeof(char *));
  res[0] = bdd_bin;

  char *arg1 = strtok(line, " ");
  res[1] = arg1;
  char *arg2 = strtok(NULL, " ");
  res[2] = arg2;
  if (arg2 == NULL)
  {
    arg1[strlen(arg1)] = '\0';
    return res;
  }

  char *arg3 = strtok(NULL, " ");
  res[3] = arg3;
  if (arg3 == NULL)
  {
    arg2[strlen(arg2)] = '\0';
    return res;
  }

  char *arg4 = strtok(NULL, " ");
  res[4] = arg4;
  if (arg4 == NULL)
  {
    arg3[strlen(arg3)] = '\0';
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
void *process_communication(void *new_socket_ptr)
{
  reader_sem = sem_open(READER_SEM_NAME, O_CREAT, 0644, MAX_READER);
  if (reader_sem == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }
  if (sem_close(reader_sem) < 0)
  {
    sem_unlink(READER_SEM_NAME);
    perror("sem_close");
    exit_msg("sem_close", 1);
  }

  writer_sem = sem_open(WRITER_SEM_NAME, O_CREAT, 0644, 1);
  if (writer_sem == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }
  if (sem_close(writer_sem) < 0)
  {
    sem_unlink(WRITER_SEM_NAME);
    perror("sem_close");
    exit_msg("sem_close", 1);
  }

  log_sem = sem_open(LOG_SEM_NAME, O_CREAT, 0644, 1);
  if (log_sem == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }
  if (sem_close(log_sem) < 0)
  {
    sem_unlink(LOG_SEM_NAME);
    perror("sem_close");
    exit_msg("sem_close", 1);
  }

  int new_socket = *(int *)new_socket_ptr;

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
    free(args);
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
      if (strcmp(args[1], "SEE") == 0 || strcmp(args[1], "SEE_DAY") == 0 || strcmp(args[1], "SEE_USER") == 0)
      {
        char replyBuffer[REPLY_SIZE];
        ssize_t nbytes = read(pipefd[0], replyBuffer, REPLY_SIZE - 1);
        if (nbytes >= 0)
        {
          replyBuffer[nbytes] = '\0';
          write(new_socket, replyBuffer, REPLY_SIZE);
        }
        close(pipefd[0]);
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
    free(args);
  }

  return NULL;
}

int main()
{
  signal(SIGINT, signal_handler);

  // Configuration de la socket serveur
  socket_desc = configure_socket();

  // Réception des connections réseaux entrantes
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(SERVER_IP);
  address.sin_port = htons(SERVER_PORT);
  socklen_t addrlen = sizeof(address);

  // Gestion des commandes entrantes dans la nouvelle socket
  while (1)
  {
    printf("En attente de connexion\n");
    int *new_socket_ptr = malloc(sizeof(int));
    *new_socket_ptr = accept(socket_desc, (struct sockaddr *)&address, &addrlen);
    if (*new_socket_ptr < 0)
    {
      free(new_socket_ptr);
      continue;
    }
    printf("Connection établie\n");

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, *process_communication, new_socket_ptr) < 0)
    {
      perror("pthread_create");
      exit_msg("Thread creation failed", 1);
    }
  }
}
