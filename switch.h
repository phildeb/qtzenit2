#if 0

#ifndef _SWITCH_H_
#define _SWITCH_H_


/*
  une board est definie par 
  - son emplacement
  - son type
  - sa configuration matricielle
  - son etat
  - les postes reliés
   /vega/bin:
   - conf : liste des cartes qui doivent etre presentes dans le meuble
   - conf1.csv : decrit la config matricielle statique pour la premiere conference
   chaque ligne represente : board in, port in, channel (sous channel), board out, port out, time slot
   - pour les fichiers jupiter et radio c'est la meme chose !
   - conf2.
exemple
19;2;1;19;3;32;

ici ca signifie que la sortie carte 19, port 2 (in pour eux) est connecté à la carte 19 port 3 sous channel d'entree 1 (sortie en fait..)
a travers le tsslot 32.

   - mixeurs.csv : fichier de configuration de la carte de mix des conferences (mix des 3 autres conferences);
 */



typedef enum {
  board_aslt = 0,
  board_aga,
  board_alphacom
} board_type_t;

typedef struct board_configuration_s {
  int place;
  int present;
  board_type_t type;    
 
} board_configuration_t;

typedef struct board_s {
  int maintenance_state;
  
  board_configuration_t configuration;
} board_t;


/* le switch est en fait l'ensemble des cartes avec le fond de panier */
typedef enum {
  switch_event_board_initial_presence = 0,
  switch_event_board_extracted,
  switch_event_board_inserted,
  switch_event_board_anomaly
} switch_event_t;

typedef struct switch_s {
  board_t boards[MAX_BOARD_NUMBER];
  int nb_boards;
} switch_t;

void init_switch();		

#endif

#endif
