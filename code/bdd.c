#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "bdd.h"
#include "utils.h"

sem_t *reader_sem;
sem_t *writer_sem;
sem_t *log_sem;

// Nom du fichier contenant les données
static const char *DATA = "data";
static const char *LOG = "data.log";
static const char *TMP_ADD = "tmp_add";
static const char *TMP_DEL = "tmp_del";

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
  default:
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
  for (size_t i = 0; i < strlen(d); i++)
    d[i] = (char)tolower((int)d[i]);

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
  {
    free(l);
    exit_msg(error_msg, 0);
  }
  data->name = malloc(strlen(parse) + 1);
  strcpy(data->name, parse);

  parse = strtok(NULL, ",");
  if (parse == NULL)
  {
    free(l);
    exit_msg(error_msg, 0);
  }
  data->activity = malloc(strlen(parse) + 1);
  strcpy(data->activity, parse);

  parse = strtok(NULL, ",");
  if (parse == NULL)
  {
    free(l);
    exit_msg(error_msg, 0);
  }
  data->day = string_to_day(parse);

  parse = strtok(NULL, "\n");
  if (parse == NULL)
  {
    free(l);
    exit_msg(error_msg, 0);
  }
  data->hour = atoi(parse);
  free(l);

  return data;
}

// La fonction _add_data_  retourne 0 si l'opération s'est bien déroulé
// sinon -1
int add_data(Data *data)
{
  int inserted = 0; // inserted = 0 -> pas encore inséré; 1 -> inséré; 2 -> déjà présent
  FILE *f = fopen(DATA, "r+");
  if (f == NULL)
    return -1;
  FILE *tmp = fopen(TMP_ADD, "w");
  if (tmp == NULL)
    return -1;
  char line[LINE_SIZE];
  while (fgets(line, LINE_SIZE, f) != NULL)
  {
    Data *d = get_data(line);
    // On insère la donnée dans l'ordre croissant
    if (inserted == 0 && (d->day > data->day || (d->day == data->day && d->hour >= data->hour)))
    {
      if (d->day == data->day && d->hour == data->hour && strcmp(d->name, data->name) == 0)
      {
        inserted = 2;
      }
      else
      {
        char new_line[LINE_SIZE];
        data_format(new_line, data);
        fprintf(tmp, "%s", new_line);
        inserted = 1;
      }
    }
    fprintf(tmp, "%s", line);
    data_free(d);
  }
  if (inserted == 0)
  {
    data_format(line, data);
    fprintf(tmp, "%s", line);
  }
  if (fclose(f) == EOF)
    return -1;
  if (fclose(tmp) == EOF)
    return -1;
  if (remove(DATA) != 0)
    return -1;
  if (rename(TMP_ADD, DATA) != 0)
    return -1;
  if (inserted == 2)
    return 2;
  return 0;
}

// Enlève la donnée _data_ de la base de donnée
int delete_data(Data *data)
{
  FILE *f = fopen(DATA, "r+");
  if (f == NULL)
    return -1;
  FILE *tmp = fopen(TMP_DEL, "w");
  if (tmp == NULL)
    return -1;
  char line[LINE_SIZE];
  while (fgets(line, LINE_SIZE, f) != NULL)
  {
    Data *d = get_data(line);
    // On réécrit toutes les lignes sauf celle qu'on veut supprimer
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
  if (remove(DATA) != 0)
    return -1;
  if (rename(TMP_DEL, DATA) != 0)
    return -1;
  return 0;
}

// Affiche le planning
char *see_all(char *answer)
{
  FILE *f = fopen(DATA, "r");
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

// Affiche le planning d'un jour
char *see_day(char *answer, char *day_string)
{
  enum Day day = string_to_day(day_string);
  if (day == NONE)
    return "";
  FILE *f = fopen(DATA, "r");
  if (f == NULL)
    return "";
  char line[LINE_SIZE];
  sprintf(answer, "Planning du %s:\n", day_to_string(day));
  printf("%s", answer);
  while (fgets(line, LINE_SIZE, f) != NULL)
  {
    Data *d = get_data(line);
    if (d->day == day)
    {
      sprintf(answer, "%dh: %s a %s\n", d->hour, d->name, d->activity);
      printf("%s", answer);
    }
    data_free(d);
  }
  if (fclose(f) == EOF)
    return "";
  return answer;
}

// Affiche le planning d'un utilisateur
char *see_user(char *answer, char *user)
{
  FILE *f = fopen(DATA, "r");
  if (f == NULL)
    return "";
  char line[LINE_SIZE];
  sprintf(answer, "Planning de %s:\n", user);
  printf("%s", answer);
  while (fgets(line, LINE_SIZE, f) != NULL)
  {
    Data *d = get_data(line);
    if (strcmp(d->name, user) == 0)
    {
      sprintf(answer, "%s %dh: %s\n", day_to_string(d->day), d->hour, d->activity);
      printf("%s", answer);
    }
    data_free(d);
  }
  if (fclose(f) == EOF)
    return "";
  return answer;
}

// Ajoute une ligne de log
int add_log(char **command, int nwords)
{
  char *log = concatenate_array(command, nwords);
  FILE *f = fopen(LOG, "a");
  if (f == NULL)
    return -1;
  fprintf(f, "%s\n", log);
  free(log);
  if (fclose(f) == EOF)
    return -1;
  return 0;
}

int main(int argc, char **argv)
{
  sem_t *reader_sem = sem_open(READER_SEM_NAME, O_RDWR);
  if (reader_sem == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }
  sem_t *writer_sem = sem_open(WRITER_SEM_NAME, O_RDWR);
  if (writer_sem == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }
  sem_t *log_sem = sem_open(LOG_SEM_NAME, O_RDWR);
  if (log_sem == SEM_FAILED)
  {
    perror("sem_open");
    exit_msg("sem_open", 1);
  }

  // On ajoute la commande dans le log
  sem_wait(log_sem);
  add_log(argv, argc);
  sem_post(log_sem);
  sem_close(log_sem);

  if (argc == 6) // ADD ou DEL
  {
    // On crée une structure Data à partir des arguments
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
      sem_wait(writer_sem);
      sem_wait(reader_sem);
      int value;
      while (1)
      {
        sem_getvalue(reader_sem, &value);
        if (value == MAX_READER - 1)
        {
          break;
        }
      }
      int res = -add_data(d);
      sem_post(reader_sem);
      sem_post(writer_sem);
      sem_close(reader_sem);
      sem_close(writer_sem);
      data_free(d);
      return res;
    }
    else if (strcmp(action, "DEL") == 0)
    {
      sem_wait(writer_sem);
      sem_wait(reader_sem);
      int value;
      while (1)
      {
        sem_getvalue(reader_sem, &value);
        if (value == MAX_READER - 1)
        {
          break;
        }
      }
      int res = -delete_data(d);
      sem_post(reader_sem);
      sem_post(writer_sem);
      sem_close(reader_sem);
      sem_close(writer_sem);
      data_free(d);
      return res;
    }
    else
    {
      sem_close(reader_sem);
      sem_close(writer_sem);
      data_free(d);
      return 1;
    }
  }
  else if (argc == 2 && strcmp(argv[1], "SEE") == 0)
  {
    char *answer = malloc(LINE_SIZE);
    sem_wait(writer_sem);
    sem_wait(reader_sem);
    sem_post(writer_sem);
    int res = strlen(see_all(answer)) == 0;
    sem_post(reader_sem);
    sem_close(reader_sem);
    sem_close(writer_sem);
    free(answer);
    return res;
  }
  else if (argc == 3) // SEE_DAY ou SEE_USER
  {
    if (strcmp(argv[1], "SEE_DAY") == 0)
    {
      char *answer = malloc(LINE_SIZE);
      sem_wait(writer_sem);
      sem_wait(reader_sem);
      sem_post(writer_sem);
      int res = strlen(see_day(answer, argv[2])) == 0;
      sem_post(reader_sem);
      sem_close(reader_sem);
      sem_close(writer_sem);
      free(answer);
      return res;
    }
    else if (strcmp(argv[1], "SEE_USER") == 0)
    {
      char *answer = malloc(LINE_SIZE);
      sem_wait(writer_sem);
      sem_wait(reader_sem);
      sem_post(writer_sem);
      int res = strlen(see_user(answer, argv[2])) == 0;
      sem_post(reader_sem);
      sem_close(reader_sem);
      sem_close(writer_sem);
      free(answer);
      return res;
    }
    else
    {
      sem_close(reader_sem);
      sem_close(writer_sem);
      return 1;
    }
  }
  else // Commande inconnue
  {
    sem_close(reader_sem);
    sem_close(writer_sem);
    return 1;
  }
  return 0;
}
