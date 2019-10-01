#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>

#include "conf.h"
#include "switch.h"

extern switch_t backpanel;

void init_switch()		/* todo : pourvoir charger une configuration */
{
  int i = 0;

  bzero(&backpanel, sizeof(backpanel));

  backpanel.boards[i].configuration.present = 1; /* board ASLT 1 postes CRM4 numero D1 a D6 */
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 1;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.boards[i].configuration.present = 1; /* board ASLT 2 postes CRM4 numero D7 a D12 */
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 2;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.boards[i].configuration.present = 0; /* board ASLT 3 postes CRM4 numero D13 a D18 */
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 3;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.boards[i].configuration.present = 0; /* board ASLT 4 postes CRM4 numero D19 a D24 */
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 4;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.boards[i].configuration.present = 0; /* board AGA 15 */
  backpanel.boards[i].configuration.type = board_aga;
  backpanel.boards[i].configuration.place = 15;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.boards[i].configuration.present = 1; /* board AGA 16 pdl20090609 */
  backpanel.boards[i].configuration.type = board_aga;
  backpanel.boards[i].configuration.place = 16;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.boards[i].configuration.present = 1; /* board AGA 17 */
  backpanel.boards[i].configuration.type = board_aga;
  backpanel.boards[i].configuration.place = 17;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board AGA 18 */
  backpanel.boards[i].configuration.type = board_aga;
  backpanel.boards[i].configuration.place = 18;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board AGA 19 */
  backpanel.boards[i].configuration.type = board_aga;
  backpanel.boards[i].configuration.place = 19;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board AGA 20 */
  backpanel.boards[i].configuration.type = board_aga;
  backpanel.boards[i].configuration.place = 20;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board AGA 22 */
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 22;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board AGA 23 */
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 23;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board AGA 24*/
  backpanel.boards[i].configuration.type = board_aslt;
  backpanel.boards[i].configuration.place = 24;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;
  backpanel.boards[i].configuration.present = 1; /* board ALPHACOM */
  backpanel.boards[i].configuration.type = board_alphacom;
  backpanel.boards[i].configuration.place = 25;
  //vega_alarm_log(EV_ALARM_BOARD_ANOMALY, backpanel.boards[i].configuration.place, 0, "Anomalie sur la carte %d", backpanel.boards[i].configuration.place); // pdl: carte rouge sur ihm  
  i++;

  backpanel.nb_boards = i;
 

  return;
}


/* 
   ici, on gere des evenements sur le backpanelitch :
   - presence initiale d'une carte
   - anomalie d'une carte
   - retire une carte
   - insere une carte
*/
/* int switch_handle_event(switch_t *backpanel, switch_event_t event, BYTE *value, int value_len) */
/* { */
/*   backpanelitch (event) { */
/*   case backpanelitch_event_board_initial_presence: */
/*     break; */
/*   case backpanelitch_event_board_anomaly: */
/*     /\* checker pour chaque conference si il y un abonné sur la carte en anomalie *\/ */
/*     /\* placer un display rouge sur la carte  *\/ */
/*     break; */
/*   case backpanelitch_event_board_inserted: */
/*     /\* checker pour chaque conferene si il y un abonné sur la carte inserée *\/ */
/*     /\* placer un display vert sur la carte *\/ */
/*     break; */
/*   case backpanelitch_event_board_extracted: */
/*     /\* checker pour chaque conference si il y un abonné sur la carte extraite *\/ */
/*     break; */
/*   default: */
/*     break; */
/*   } */

/*   return 0; */
/* } */




/* IDEE: faire des evenmenet uniquement sur ports.

propager les evenements de ports sur les devices et les conferences.
avoir une configuration par port des cartes.
exemple utilisation : 

pour la carte 1 qui est une carte aslt, on va :

*** evenement presencence initiale d'un port abonné
===> créer un device sur ce port puisque que ce port est configuré comme port 
abonné.

===> propager la presence initiale du port à toutes les conferences:
	- si pour une configuration de conference , il y a ce numéro de device
	dans la liste, alors on le place dans la liste des devices connectés.
       
*** evement presence port AGA mixer de configuration
==> propager l'evenment initiale de presence du port à la conference:
	- si le port est un des ports ressources d'une conference, alors on le place comme present.

****  presence initiale d'un port jupiter_in :
===> créer un device avec le numéro d'abonné associé au port si le device n'existe pas.
===> propager la presence initiale du port à toutes les conferences:
        - si pour une configuration de conference donnée on utilise ce device, 
	alors le device est declaré comme present.


*** evenement anomalie port aslt abonné:
===> trouver le device sur ce port, le declaré comme en anomalie
===> propager l'evenement "anomalie device" à toutes les conferences:
	- si le device fait partie d'une conference, alors on le declaré non present (en anomalie).



 */
#endif
