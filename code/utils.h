#include <stdlib.h>
#include <stdio.h>

const char *SERVER_IP = "127.0.0.1"; // Adresse localhost (interface interne)
#define SERVER_PORT 5000
#define LINE_SIZE 100
#define REPLY_SIZE 2000
#define DEBUG 0
#define READER_SEM_NAME "/reader-sem"
#define WRITER_SEM_NAME "/writer-sem"
#define LOG_SEM_NAME "/log-sem"
#define MAX_READER 10

// Renvoie un message d'erreur en cas de problème
void exit_msg(char *msg, int err)
{
  fprintf(stderr, "Error - %s: ", msg);
  if (err)
    perror(NULL);
  exit(1);
}

char *concatenate_array(char **array, int nwords)
{
  char *result = malloc(LINE_SIZE);
  result[0] = '\0';
  for (int i = 1; i < nwords - 1; i++)
  {
    strcat(result, array[i]);
    strcat(result, " ");
  }
  strcat(result, array[nwords - 1]);
  return result;
}
