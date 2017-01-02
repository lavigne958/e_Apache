#ifndef VIGILANTE_H
#define VIGILANTE_H

#include <pthread.h>
#define BLOCKED 1

/* Structure représentant une liste chaînée des clients connectés au serveur */
typedef struct client_data_count{
  int track[60];                    /* tableau contenant la quantite de données envoyées au client pour chaque seconde */
  long ip;                          /* adresse ip du client */
  struct client_data_count *next;   /* element suivant dans la liste chainée */
  int flag;       
  int last;                         /* dernière seconde à laquelle le client a reçu des données */
  int timeleft;                     /* variable utilisée quand un client est bloqué, contient le temps restant avant debloquage */
}client_data_count;


/* Structure passée en paramètre du thread vigilante, 
   contenant les informations nécessaires à son fonctionnement */
typedef struct vigilante{
  pthread_t tid;                /* id du thread vigilante lors d'un appel à create_vigilante_thread */
  int threshold;                /* seuil représentant la quantité de données pouvant être reçues par un client par minute */
  pthread_mutex_t mutex;
  client_data_count *clients;   /* liste chaînée de tous les clients s'étant connectés au serveur */
}vigilante;



/* Crée un thread vigilante qui va permettre de surveiller la quantité de données 
   envoyées à tous les clients du serveur. La méthode renvoie l'adresse de la structure
   vigilante passée en paramètre au thread afin de pouvoir communiquer avec lui
*/
vigilante *create_vigilante_thread(int threshold);


/* Ajoute un client dans la liste des clients s'étant connectés au serveur */
void add_client(vigilante* vigil, long ip);


/* Retourne la quantité de données envoyées au client dans la dernière minute */
int check_total(client_data_count *client);


/* Cette méthode va vérifier que le client d'adresse ip "ip" a le droit
   de recevoir des données puis va écrire le contenu de size dans la case du champs 
   track correspondant à la seconde courante.
*/
int incremente_size(vigilante *v, int size, long ip);


/* Cette méthode est celle appelée par le thread vigilante toutes les secondes.
   Pour chaque client, si aucune requête n'a été emise par le client durant la seconde 
   courante, cette méthode met la case du champs track correspondante
   à zero. Si l'on trouve un client bloqué on décrémente le champs timeleft
   et on débloque le client si nécessaire.
*/
void check_clients(vigilante *v);


/* Retourne 0 si le client d'adresse ip "ip" n'est pas bloqué */
int is_blocked(vigilante *v, long ip);


/* Méthode exécutée par le thread vigilante, elle va juste faire un appel à check_client 
   toutes les secondes 
*/
void *vigilante_thread(void *vigil);


#endif
