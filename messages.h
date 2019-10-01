#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "const.h"

/* get software mode (slave ou master */
#define		ICCMT_CC_GET_SW_VSN	0x59

#define		ICCMT_CC_SW_VSN		0x5a

/* message permettant de basculer en mode esclave */

#define		ICCMT_CC_SET_MODE	0x58


/* end of message flag */

#define		EOMSG		      	0xF1


/* ping / pong */

#define		ICCMT_CC_PING		0x56
#define		ICCMT_CC_PONG		0x57	



/* scan d'une carte  */
#define ICCMT_BOARD_SIMPLE_SCAN		0x4a
#define ICCMT_BOARD_AVAILABLE		0x4c
#define ICCMT_STATION_UNAVAILABLE		0x46 // pdl 20091002

/* evenement carte indisponible  */

#define ICCMT_BOARD_NOT_AV		0x4e

/* efface les erreurs */
#define ICCMT_BOARD_CLEAR_ERROR		0x52

/* led verte activation */
#define ICCMT_BOARD_CLEAR_BUSY		0x50

/* activation mode occupe pour l'alpha com  */
#define ICCMT_BOARD_SET_BUSY		0x51

/*************** time slot messages *****************/

/* connexion du subchannel 0 vers un timeslot */
#define ICCMT_TS_2_SC0_CONN		0x03

/* connexion du subchannel 1 vers un timeslot */
#define ICCMT_TS_2_SC1_CONN		0x04

/* connexion du subchannel 3 vers un timeslot */
#define ICCMT_TS_2_SC3_CONN		0x06

/* deconnexion du subchannel 0 vers un timeslot */
#define ICCMT_TS_2_SC0_DISC		0x09

/* deconnexion du subchannel 1 vers un timeslot */
#define ICCMT_TS_2_SC1_DISC		0x0a

/* deconnexion du subchannel 3 vers un timeslot */
#define ICCMT_TS_2_SC3_DISC		0x0c

/* connexion d'une entrée à une sortie */
#define ICCMT_TS_2_UNITS_CONN		0x18


/* deconnexion d'une entrée vers un timeslot */

#define ICCMT_UNIT_2_TS_DISC		0x02


/***************************************************/


/* activation sn1, sn2 et sn3 */

#define ICCMT_SWITCH_SET		0x46

/* desactivation sn1, sn2 et sn3 */

#define ICCMT_SWITCH_CLEAR		0x45


/* actions postes : micro */

/* ouverture micro */
#define ICCMT_ST_OPEN_MIC		0x8c
/* fermeture micro */
#define ICCMT_ST_SHUT_MIC		0x8d
/* active les evenement sur un poste */
#define ICCMT_ST_EV_ON			0x80
/* place une nouvelle valeur de gain */
#define ICCMT_ST_SET_VOLUME		0x84


/* anomalie sur une carte */
#define ICCMT_BOARD_NOT_AVAILABLE	0x4E


/* appui touche sur un poste */

#define ICCMT_BP_MSG_ICCO_UNIT		0x8E


/* place option poste avec afficheur pour les postes CRM4 */

#define ICCMT_ST_OPTIONS		0x9D

/*
From: Hans van Dop
Sent: vendredi 9 janvier 2009 10:08
To: Fran�ois Weppe
Subject: Side tone OFF
Hi Fran�ois,
 
There is a command to switch off the side tone:
SYNONYM ICCMT_ST_SIDE_OFF          ICC_MSG_TYPE = 138;  (HEX--> 0x8A)
This message has 1 parameter: the port number
 */

#define ICCMT_ST_SIDE_OFF			0x8A
//#define ICCMT_ST_SIDE_OFF			0x8B




/* busy tone */
#define		TS_CONFERENCE_TONE			0x01
#define		TS_BUSY_TONE				0x06
#define		TS_GENERAL_CALL_TONE		0x08
#define		TS_RING_TONE				0x02
#define		TS_ATT_TONE					0x03
#define		TS_ACTIVATION_CONF_TONE		0x04
#define		TS_DTMF1			0x09

/* message en reception */
typedef struct tcp_msg_s {
  //int rx_seq_num;
  int size;
  unsigned char data[MAX_MSG_SIZE];
} tcp_msg_t;


typedef enum {
  ALPHACOM_CRM4_INPUT=0,//event_device_input = 0,
  ALPHACOM_LINK_UP,//event_link_up, 	/* connecté en mode slave et  fonctionnel activé */
  //EVENT_BOARD_ANOMALY,
  //EVENT_BOARD_END_ANOMALY,
  //event_port_anomaly,
  //event_port_enabled,
  ALPHACOM_TIMER,//event_tick_100_milliseconds,
  ALPHACOM_PING_SENT,//event_ping_sent
} vega_event_type_t ;

typedef struct vega_input_event_s {
  int board_number;
  int input_number;
} vega_input_event_t;

typedef struct vega_board_end_anomaly_s {
  int board_number;
} vega_board_end_anomaly_t;


typedef struct vega_board_anomaly_s {
  int board_number;
} vega_board_anomaly_t;


typedef struct vega_event_s {
  vega_event_type_t type;

  union {
    vega_input_event_t input_event;
    vega_board_anomaly_t board_anomaly;
    vega_board_end_anomaly_t board_end_anomaly;
  } event;

} vega_event_t;
 
const char* dump_message(tcp_msg_t *pmsg);

const char *dump_message_buffer(unsigned char * buf, int size);

const char *dump_message_serialized(unsigned char *buf, int size);


#endif
