//Enum pour les jours de la semaine et ses fonctions
enum Day { MON,TUE, WED, THU, FRI, SAT, SUN, NONE};
char* day_to_string(enum Day d);
enum Day string_to_day(char* d);

//Structure pour les différents champs d'une donnée
struct Data {
  char* name;
  char* activity;
  enum Day day;
  int hour;
};
typedef struct Data Data;

void data_free(Data* d);
void data_format(char* l, Data* data);
