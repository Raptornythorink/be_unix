#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bdd.h"
#include "utils.h"

// Nom du fichier contenant les données
static const char *DATA = "data";

// Retourne une string à partir d'un Day
char *day_to_string(enum Day d)
{
  switch (d)
  {
  case MON:
    return "Lundi";
  case TUE:
    return "Mardi";
  case WED:
    return "Mercredi";
  case THU:
    return "Jeudi";
  case FRI:
    return "Vendredi";
  case SAT:
    return "Samedi";
  case SUN:
    return "Dimanche";
  case NONE:
    return "Not a day";
  }
}

// Retourne un Day à partir d'un string
// dans le cas où la string ne correspond pas à un jour, on renvoie NONE
enum Day string_to_day(char *dd)
{
  char d[LINE_SIZE];
  strcpy(d, dd);
  // Conversion en minuscule
  for (int i = 0; i < strlen(d); i++)
    d[i] = tolower(d[i]);

  if (strcmp("lundi", d) == 0)
    return MON;
  else if (strcmp("mardi", d) == 0)
    return TUE;
  else if (strcmp("mercredi", d) == 0)
    return WED;
  else if (strcmp("jeudi", d) == 0)
    return THU;
  else if (strcmp("vendredi", d) == 0)
    return FRI;
  else if (strcmp("samedi", d) == 0)
    return SAT;
  else if (strcmp("dimanche", d) == 0)
    return SUN;
  else
    return NONE;
}

// Libère la mémoire d'un pointeur vers Data
void data_free(Data *d)
{
  free(d->name);
  free(d->activity);
  free(d);
}

// Modifie une chaîne de caratère correspondant à data
void data_format(char *l, Data *data)
{
  sprintf(l, "%s,%s,%s,%d\n",
          data->name, data->activity,
          day_to_string(data->day), data->hour);
}

// Retourne une structure Data à partir d'une ligne de donnée
//  get_data("toto,arc,lundi,4") ->  Data { "toto", "arc", MON, 4 };
//  Attention il faudra libérer la mémoire vous-même après avoir utilisé
//  le pointeur généré par cette fonction
Data *get_data(char *line)
{
  char *parse;
  Data *data = malloc(sizeof(Data));
  char error_msg[LINE_SIZE];
  sprintf(error_msg, "Erreur de parsing pour: %s\n", line);

  // On s'assure que la ligne qu'on parse soit dans le mémoire autorisée en
  //  écriture
  char *l = malloc(strlen(line) + 1);
  l = strncpy(l, line, strlen(line) + 1);

  parse = strtok(l, ",");
  if (parse == NULL)
    exit_msg(error_msg, 0);
  data->name = malloc(strlen(parse) + 1);
  strcpy(data->name, parse);

  parse = strtok(NULL, ",");
  if (parse == NULL)
    exit_msg(error_msg, 0);
  data->activity = malloc(strlen(parse) + 1);
  strcpy(data->activity, parse);

  parse = strtok(NULL, ",");
  if (parse == NULL)
    exit_msg(error_msg, 0);
  data->day = string_to_day(parse);

  parse = strtok(NULL, "\n");
  if (parse == NULL)
    exit_msg(error_msg, 0);
  data->hour = atoi(parse);
  free(l);

  return data;
}

// La fonction _add_data_  retourne 0 si l'opération s'est bien déroulé
// sinon -1
int add_data(Data *data)
{
  FILE *f = fopen("data", "a");
  if (f == NULL)
    return -1;
  fprintf(f, "%s,%s,%s,%d\n", data->name, data->activity,
          day_to_string(data->day), data->hour);
  if (fclose(f) == EOF)
    return -1;
  return 0;
}

// Enlève la donnée _data_ de la base de donnée
int delete_data(Data *data)
{
  FILE *f = fopen("data", "r+");
  if (f == NULL)
    return -1;
  FILE *tmp = fopen("tmp", "w");
  if (tmp == NULL)
    return -1;
  char line[LINE_SIZE];
  while (fgets(line, LINE_SIZE, f) != NULL)
  {
    Data *d = get_data(line);
    if (strcmp(data->name, d->name) != 0 || strcmp(data->activity, d->activity) != 0 || data->day != d->day || data->hour != d->hour)
    {
      fprintf(tmp, "%s", line);
    }
    data_free(d);
  }
  if (fclose(f) == EOF)
    return -1;
  if (fclose(tmp) == EOF)
    return -1;
  if (remove("data") != 0)
    return -1;
  if (rename("tmp", "data") != 0)
    return -1;
  return 0;
}

// Affiche le planning
char *see_all(char *answer)
{
  FILE *f = fopen("data", "r");
  if (f == NULL)
    return "";
  char line[LINE_SIZE];
  sprintf(answer, "Planning:\n");
  printf("%s", answer);
  while (fgets(line, LINE_SIZE, f) != NULL)
  {
    Data *d = get_data(line);
    sprintf(answer, "%s %dh: %s a %s\n", day_to_string(d->day), d->hour, d->name, d->activity);
    printf("%s", answer);
    data_free(d);
  }
  if (fclose(f) == EOF)
    return "";
  return answer;
}

int main(int argc, char **argv)
{
  sem_t *semaphore = sem_open(SEM_NAME, O_RDWR);
  if (semaphore == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }

  if (argc == 6)
  {
    Data *d = malloc(sizeof(Data));
    d->name = malloc(strlen(argv[2]));
    strcpy(d->name, argv[2]);
    d->activity = malloc(strlen(argv[3]));
    strcpy(d->activity, argv[3]);
    d->day = string_to_day(argv[4]);
    d->hour = atoi(argv[5]);
    char *action = argv[1];
    if (strcmp(action, "ADD") == 0)
    {
      sem_wait(semaphore);
      int res = -add_data(d);
      sem_post(semaphore);
      sem_close(semaphore);
      data_free(d);
      return res;
    }
    else if (strcmp(action, "DEL") == 0)
    {
      sem_wait(semaphore);
      int res = -delete_data(d);
      sem_post(semaphore);
      sem_close(semaphore);
      data_free(d);
      return res;
    }
    else
    {
      data_free(d);
      sem_close(semaphore);
      return 1;
    }
  }
  else if (argc == 2)
  {
    char *answer = malloc(LINE_SIZE);
    sem_wait(semaphore);
    int res = strlen(see_all(answer)) == 0;
    sem_post(semaphore);
    sem_close(semaphore);
    free(answer);
    return res;
  }
  else
  {
    return 1;
  }
  return 0;
}
