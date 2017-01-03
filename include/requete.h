#ifndef REQUET_H
#define REQUET_H

#include "server.h"
#include "vigilante.h"

#define NOT_FOUND 404
#define FORBIDDEN 403
#define OK 200
#define INTERNAL_ERROR 500
#define SIZE_REQUEST 4096

typedef struct thread_fils{
  int *counter;             /* variable contenant le numero du thread qui peut ecrire sur la socket */
  int id;                   /* numero du thread */
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
  client *cli;              /* structure contenant des infos sur le client (voir server.h) */
  char get[SIZE_REQUEST];   /* champ contenat la ligne du GET de la requête */ 
}thread_fils;


/* Cette méthode renvoie dans la chaine passée en paramètre la date
   courante sous le format day/month/year-hour:min:sec
*/
char* get_time(char* result);



/* Cette méthode vérifie que le fichier existe et qu'il est accessible par le serveur en
   lecture. Il renvoie dans les paramètres code et string, le code et le message
   de retour HTTP correspondant. Si le fichier est exécutable le code est égal à 0.
*/
void check_file(const char *pathname, int* code, char* string);




/* Cette méthode recherche l'extension de la chaine chemin
   et la renvoie dans le paramètre extension
*/
char *get_extension(char *chemin, char *extension);




/* Cette méthode parse le fichier /etc/mime.types pour chercher le mime type 
   correspondant à l'extension et renvoie le resultat dans la chaine 
   mime passée en paramètres.
   Si la fonction n'a rien trouvée elle renvoie 0.
*/
int get_mime(const char *extension, char *mime);




/* Cette methode va chercher dans un fichier contenant un message de réponse
   http, le code de retour et la taille des données envoyées
*/
int get_code_and_size(const char *filename, int *code, int *length);




/*
  Cette méthode va ajouter une ligne dans le fichier de journalisation 
  avec les arguments passées en paramètres. 
*/
void write_log(client *c, char* str_get, int ret_code, int size);





/* Méthode appelée par le thread_server pour attendre que les sous-threads
   aient fini le traitement de leur requête.
*/
void wait_thread(pthread_mutex_t *mutex, int nb_threads, int *finis);




/* Cette méthode est celle exécutée par les threads créées par le serveur
   pour gérer une connection. Pour chaque requêtes, elle lira tous les headers
   et créera une thread qui s'occupera de traiter la requête.
*/
void *thread_server(void *arg);




/* Cette méthode permet la synchronisation des sous-thread 
   de traitement de requêtes.
*/
void lock_write_socket(thread_fils *fils);



/* Cette méthode permet de déverouiller le socket en écriture,
   pour la synchronisation entre les sous-thread de traitement de 
   requêtes.
*/
void unlock_write_socket(thread_fils *fils);



/* Cette méthode exécute l'exécutable de chemin pathname.
   Le processus fils qui exécutera le code ecrira le resultat de l'exécution dans 
   un fichier temporaire (code de retour HTTP + données).
   Si l'exécution se déroule en moins de MS_TO_WAIT ms, le processus père
   enverra tous le contenu du fichier au socket client, sinon il envoie 
   "HTTP/1.1 500 Iternal Error\n\n".
*/
void execute(char *pathname, thread_fils *self);



/* Cette méthode est celle exécutée par les sous-threads pour traiter 
   une requête.
*/
void *process_request(void *arg);


/* Cette méthode envoie le contenu d'un fichier au socket 
   passé en argument
*/
int send_file(int socket, const char* pathname);

#endif
