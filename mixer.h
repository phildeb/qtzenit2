#ifndef _MIXER_H
#define _MIXER_H

typedef enum {
  sub_channel_0 = 0,
  sub_channel_1 = 1,
  sub_channel_3 = 3
} sub_channel_t ;

extern void dump_timeslots(FILE*,int connected_only);

/* represente une sous entree */
typedef struct sub_output_s {
  int connected;		/* connecté ou pas */
  int tslot; 			/* numéro du time slot connecté */
} mixer_sub_output_t;

/* une entre peut etre connectee ou pas  */
typedef struct mixer_input_s {
  int connected; 		/* connecté au pas */
  int tslot;
} mixer_input_t;

typedef struct physical_mixer_s {
  /* etat sn1 : 1 == activé  0 == desactivé */
  int sn1;
  int sn2;
  int sn3;
  mixer_sub_output_t sub_channel0; /* sous sortie 0 */
  mixer_sub_output_t sub_channel1; /* sous sortie 1 */
  mixer_sub_output_t sub_channel3; /* sous sortie 3 */
  mixer_input_t input;		/* entrée */
} vega_physical_mixer_t;

typedef struct port_address_s {
  int size;
  BYTE byte1;
  BYTE byte2;
} port_address_t;

typedef struct  {
  int number;
  int board_number;
  int sbi_number;
  int ts_start;
  int conference_mixer_board_number;
  int conference_mixer_sbi_number;
  int conference_mixer_port_number;
} conference_configuration_t;

typedef struct vega_port_s  {
  int board_number;		/* numéro de carte sur laquelle est le port */
  int port_number; 			/* numero du port : sont numerotés de 0 - 7 sur un sbi */
  int sbi_number;
  int type;			/* type de port */

  port_address_t address;			/* addresse du port */
  vega_physical_mixer_t mixer;				/* etat courant du mixer */
  conference_configuration_t *conference_matrix_configuration; 		/* utile pour les ports de configuration */
  void *my_device; 		/* TODO : changer en device_t */
} vega_port_t;


int mixer_connect_input_to_sub_output(vega_port_t *input_port, vega_port_t *sub_output_port, sub_channel_t typechannel_out, int timeslot);
int mixer_connect_timeslot_to_sub_output(vega_port_t *sub_output_port, int typechannel, int timeslot, int initial);
int mixer_disconnect_timeslot_to_sub_output(vega_port_t *sub_output_port, int typechannel);
int mixer_connect_timeslot_to_input(vega_port_t *input_port,  int timeslot);

int __mixer_enable_disable__sn(vega_port_t *port, int sn_number, int enable);

#define mixer_enable_sn(a, b) __mixer_enable_disable__sn(a, b,  1);
#define mixer_disable_sn(a, b) __mixer_enable_disable__sn(a, b,  0);

port_address_t get_port_address(int board_number, int sbi_number, int port_number);
int get_board_number_from_address(port_address_t addr);
int get_port_number_from_address(port_address_t addr);

#endif
