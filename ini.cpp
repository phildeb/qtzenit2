// partie init mixers
#include "debug.h"
#include "conf.h"
#include "ini.h"
#include "misc.h"
#include "alarmes.h"

void vega_conference_t::init_mixer_conferences() // STATIC
{ 
	foreach(vega_conference_t *c,vega_conference_t::qlist){if ( c) c->init_mixers();}
}

void  device_t::init_mixer_devices() // STATIC
{
	foreach(device_t * d,device_t::qlist_devices)	{if ( d) d->init_mixers();}
}

int  CRM4::init(int board_num, int sbi_num, int port_num, 
			int conf3_board_num,int conf3_sbi_num, int conf3_port_num)
{
  memset( &port_aslt, 0 , sizeof(vega_port_t) ) ;
  memset( &port_3confs, 0 , sizeof(vega_port_t) ) ;

  board_number = board_num;
  sbi_number = sbi_num;
  port_number = port_num;

  port_aslt.board_number = board_number;
  port_aslt.sbi_number = sbi_number;
  port_aslt.port_number = port_number;
  port_aslt.address = get_port_address(board_number, sbi_number, port_number);
  port_aslt.type = VEGA_PORT_ASLT_SECONDARY;


  port_3confs.board_number = conf3_board_num;
  port_3confs.sbi_number = conf3_sbi_num;
  port_3confs.port_number = conf3_port_num;

  port_3confs.address = get_port_address(conf3_board_num, conf3_sbi_num, conf3_port_num);
  port_3confs.type = VEGA_PORT_MIXER_THREE_CONFS;
  

  // back reference to the device
  port_aslt.my_device = this;
  port_3confs.my_device = this;

  //device.crm4.keys_configuration = &CRM4::the_unic_static_device_keys_configuration ;


  m_timeslot_micro = FIRST_DEVICE_CRM4_INPUT_TS + port_aslt.port_number
	  + (get_board_number_from_address(port_aslt.address) - 1 ) * 6;

  vega_log(DEBUG, "##################### device_init_crm4 D%d name:%s %p TS%d micro", number, name, this , m_timeslot_micro);
		vega_log(DEBUG, "%03dAGA%03d-%03d-%03d....",
			(port_aslt.mixer.sub_channel1.connected   ? port_aslt.mixer.sub_channel1.tslot   : 0),
			(port_3confs.mixer.sub_channel0.connected ? port_3confs.mixer.sub_channel0.tslot : 0),
			(port_3confs.mixer.sub_channel1.connected ? port_3confs.mixer.sub_channel1.tslot : 0),
			(port_3confs.mixer.sub_channel3.connected ? port_3confs.mixer.sub_channel3.tslot : 0)
		 );

  qlist_crm4.append(this);

}



int  Radio::init_radio(int full_dupl, int board_num, int sbi_num, int port_num, int audio_det)
{

  vega_log(VEGA_LOG_DEBUG, "device_init_radio number %d %p", number, this);

  board_number = board_num;
  sbi_number = sbi_num;
  port_number = port_num;

  full_duplex = full_dupl;
  audio_detection = audio_det;

  port_aga_emission.board_number = board_number;
  port_aga_emission.sbi_number = sbi_number;
  port_aga_emission.port_number = port_number;
  port_aga_emission.type = VEGA_PORT_RADIO_EMISSION;
  port_aga_emission.address = get_port_address(board_number, sbi_number, port_number);

  port_aga_emission.my_device = this;

  port_aga_reception.board_number = board_number;
  port_aga_reception.sbi_number = sbi_number;
  port_aga_reception.port_number = port_number + 1;
  port_aga_reception.type = VEGA_PORT_RADIO_RECEPTION;
  port_aga_reception.address = get_port_address(board_number, sbi_number, port_number + 1);
  
  port_aga_reception.my_device = this;
  
  m_timeslot_micro = FIRST_DEVICE_RADIO_INPUT_TS /*121*/ + number - FIRST_RADIO_DEVICE_NUMBER /*25*/; 
 
  vega_log(VEGA_LOG_DEBUG, "##################### device_init_radio D%d name:%s %p TS%d micro detection:%d fulldupl:%d",
	   number, name, this , m_timeslot_micro, audio_detection, full_duplex);


  qlist_radio.append(this);

  /*if ( !this->has_hardware_error() ) { // pdl 20091005 est ce bien utile   cf:init_mixers() ???
	  vega_port_set_initial_mixer_config(&d->port_aga_reception); 
	  vega_port_set_initial_mixer_config(&d->port_aga_emission);
  }*/

  return 0;
}

int  Radio::init_jupiter( int board_number, int sbi_number, int port_number, int audio_det)
{
  //if (d == NULL)    return -1;
  Radio* d = this;

  vega_log(VEGA_LOG_DEBUG, "device_init_jupiter number %d %p", d->number, d);

  d->board_number = board_number;
  d->sbi_number = sbi_number;
  d->port_number = port_number;

  d->full_duplex = 1;
  d->audio_detection = audio_det;

  d->port_aga_emission.board_number = board_number;
  d->port_aga_emission.sbi_number = sbi_number;
  d->port_aga_emission.port_number = port_number;
  d->port_aga_emission.type = VEGA_PORT_JUP2_EMISSION;
  d->port_aga_emission.address = get_port_address(board_number, sbi_number, port_number);

  d->port_aga_emission.my_device = d;

  d->port_aga_reception.board_number = board_number;
  d->port_aga_reception.sbi_number = sbi_number;
  d->port_aga_reception.port_number = port_number + 1;
  d->port_aga_reception.type = VEGA_PORT_JUP2_RECEPTION;
  d->port_aga_reception.address = get_port_address(board_number, sbi_number, port_number + 1);
  
  d->port_aga_reception.my_device = d;
  

      //ts_value = FIRST_DEVICE_JUP2_INPUT_TS /*137*/ + 
		//  ((device_t*)p->my_device)->number - FIRST_JUP2_DEVICE_NUMBER; /* shift les postes jup2 commencent a 41 ! */

  //m_timeslot_micro = FIRST_DEVICE_RADIO_INPUT_TS /*121*/ + d->number - FIRST_RADIO_DEVICE_NUMBER /*25*/; 
  m_timeslot_micro = FIRST_DEVICE_JUP2_INPUT_TS + number - FIRST_JUP2_DEVICE_NUMBER;

  vega_log(VEGA_LOG_DEBUG, "##################### device_init_jupiter D%d name:%s %p TS%d micro detection:%d",
	   d->number, d->name, d , d->m_timeslot_micro, d->audio_detection);

  qlist_radio.append(d);

  return 0;
}

/* configure les mixers des ports */
int vega_conference_t::init_mixers()
{
	vega_conference_t *c = this;
	conference_configuration_t *conf = matrix_configuration;
	vega_log(INFO, "init_mixers C%d BOARD %d check PRESENCE ?",number, matrix_configuration->board_number);

	if ( has_hardware_error() ) { // pdl 20091005
		vega_log(INFO, "C%d HARDWARE ERROR !!!",conf->number);
		return -1;	
	}
	/*if ( false == map_boards[matrix_configuration->board_number] ) {
		vega_log(INFO, "init_mixers C%d BOARD %d absente ou en defaut !!!",number, matrix_configuration->board_number);
		return -1;
	}else{
		vega_log(INFO, "OK:init_mixers C%d BOARD %d",number, matrix_configuration->board_number);
	}
	if ( false == map_boards[matrix_configuration->conference_mixer_board_number] ) {
		vega_log(INFO, "init_mixers C%d MIXER BOARD %d absente ou en defaut !!!",number, matrix_configuration->conference_mixer_board_number);
		return -1;
	}else{
		vega_log(INFO, "OK:init_mixers C%d MIXER BOARD %d",number, matrix_configuration->conference_mixer_board_number);
	}*/


	for (int i = 0; i < CONF_NB_AGA_PORTS; i ++) 
	{
    switch (i) {
    case 0: 			/* 1 er port : port acces 1, 2, 3 OU peut devenir secondaire 3*/
      vega_log(VEGA_LOG_INFO, "configure port 1 ...");

      c->ports[0].type = VEGA_PORT_AGA_MIX_JUP2_RADIO1_RADIO2;
      c->ports[0].board_number = conf->board_number;
      c->ports[0].sbi_number = conf->sbi_number;
      c->ports[0].port_number = 0;
      c->ports[0].address = get_port_address(conf->board_number, conf->sbi_number, 0);
      c->ports[0].conference_matrix_configuration =  c->matrix_configuration;

      conf_vega_port_set_initial_mixer_config(&c->ports[0]);

  
      break;
    case 1:

      vega_log(VEGA_LOG_INFO, "configure port 2 ...");
      /* 2ieme port: port access 4, 5, 6  OU peut devenir secondaire4, secondaire3*/
      c->ports[1].type = VEGA_PORT_AGA_MIX_DIRECTOR_SECONDARY1_SECONDARY2;
      c->ports[1].board_number = conf->board_number;
      c->ports[1].sbi_number = conf->sbi_number;
      c->ports[1].port_number = 1;
      c->ports[1].address = get_port_address(conf->board_number, conf->sbi_number, 1);
      c->ports[1].conference_matrix_configuration =  c->matrix_configuration;

      conf_vega_port_set_initial_mixer_config(&c->ports[1]);

      break;
    case 2:			/* 3 ieme port: port radio 1 OU peut devenir secondaire5, secondaire4, secondaire3 */
      vega_log(VEGA_LOG_INFO, "configure port 3 ...");
      
  /*     if (c->radio1_device() > 0) { */
	c->ports[2].type = VEGA_PORT_AGA_RADIO1;
	c->ports[2].board_number = conf->board_number;
	c->ports[2].sbi_number = conf->sbi_number;
	c->ports[2].port_number = 2;
	c->ports[2].address = get_port_address(conf->board_number, conf->sbi_number, 2);
	c->ports[2].conference_matrix_configuration =  c->matrix_configuration;
	
	conf_vega_port_set_initial_mixer_config(&c->ports[2]);
	    
      break;
    case 3:			/* port radio 2 */
      vega_log(VEGA_LOG_INFO, "configure port 4 ...");
  /*     if (c->radio2_device() > 0){ */
	c->ports[3].type = VEGA_PORT_AGA_RADIO2;
	c->ports[3].board_number = conf->board_number;
	c->ports[3].sbi_number = conf->sbi_number;
	c->ports[3].port_number = 3;
	c->ports[3].address = get_port_address(conf->board_number, conf->sbi_number, 3);
	c->ports[3].conference_matrix_configuration =  c->matrix_configuration;
	
	conf_vega_port_set_initial_mixer_config(&c->ports[3]);
	
      break;
    case 4:			/* port jupiter 2 */
      vega_log(VEGA_LOG_INFO, "configure port 5 ...");
/*       if (c->jupiter2_device() > 0) { */
	
	c->ports[4].type = VEGA_PORT_AGA_JUPITER2;
	c->ports[4].board_number = conf->board_number;
	c->ports[4].sbi_number = conf->sbi_number;
	c->ports[4].port_number = 4;
	c->ports[4].address = get_port_address(conf->board_number, conf->sbi_number, 4);
	c->ports[4].conference_matrix_configuration =  c->matrix_configuration;
	
	conf_vega_port_set_initial_mixer_config(&c->ports[4]);
	break;


    case 5:			/* port directeur */
      vega_log(VEGA_LOG_INFO, "configure port 6 ...");
      
      c->ports[5].type = VEGA_PORT_AGA_DIRECTOR;
      c->ports[5].board_number = conf->board_number;
      c->ports[5].sbi_number = conf->sbi_number;
      c->ports[5].port_number = 5;
      c->ports[5].address = get_port_address(conf->board_number, conf->sbi_number, 5);
      c->ports[5].conference_matrix_configuration =  c->matrix_configuration;
      
      conf_vega_port_set_initial_mixer_config(&c->ports[5]);

      break;
    case 6:			/* port secondaire 1 */
      vega_log(VEGA_LOG_INFO, "configure port 7 ...");
      c->ports[6].type = VEGA_PORT_AGA_SECONDARY1;
      c->ports[6].board_number = conf->board_number;
      c->ports[6].sbi_number = conf->sbi_number;
      c->ports[6].port_number = 6;
      c->ports[6].address = get_port_address(conf->board_number, conf->sbi_number, 6);
      c->ports[6].conference_matrix_configuration =  c->matrix_configuration;
      
      conf_vega_port_set_initial_mixer_config(&c->ports[6]);

      break;
    case 7:			/* port secondaire 2 */
      vega_log(VEGA_LOG_INFO, "configure port 8 ...");

      c->ports[7].type = VEGA_PORT_AGA_SECONDARY2;
      c->ports[7].board_number = conf->board_number;
      c->ports[7].sbi_number = conf->sbi_number;
      c->ports[7].port_number = 7;
      c->ports[7].address = get_port_address(conf->board_number, conf->sbi_number, 7);
      c->ports[7].conference_matrix_configuration =  c->matrix_configuration;
      
      conf_vega_port_set_initial_mixer_config(&c->ports[7]);


      break;
    case 8:			/* port mixeur de la conference */
      vega_log(VEGA_LOG_INFO, "configure port 9 ...");

      c->ports[8].type = VEGA_PORT_AGA_MIX_CONF;
      c->ports[8].board_number = conf->conference_mixer_board_number;
      c->ports[8].sbi_number = conf->conference_mixer_sbi_number;
      c->ports[8].port_number = conf->conference_mixer_port_number;
      c->ports[8].address = get_port_address(conf->conference_mixer_board_number, conf->conference_mixer_sbi_number, conf->conference_mixer_port_number);
      c->ports[8].conference_matrix_configuration =  c->matrix_configuration;
      
      conf_vega_port_set_initial_mixer_config(&c->ports[8]);

      break;

      /* TODO : faire le port mixer de 3 conference  ou bien plutot au niveau des postes ???? */
    default:
      break;
    }
    
  }

  c->speaking_ressources[0].reservation_type = PORT_RESERVED_SECONDARY1;
  c->speaking_ressources[1].reservation_type = PORT_RESERVED_SECONDARY2;
  c->speaking_ressources[2].reservation_type = PORT_RESERVED_JUPITER2;
  c->speaking_ressources[3].reservation_type = PORT_RESERVED_RADIO1;
  c->speaking_ressources[4].reservation_type = PORT_RESERVED_RADIO2;

  c->speaking_ressources[0].busy = 0;
  c->speaking_ressources[1].busy = 0;
  c->speaking_ressources[2].busy = 0;
  c->speaking_ressources[3].busy = 0;
  c->speaking_ressources[4].busy = 0;

  c->speaking_ressources[0].device_number = 0;
  c->speaking_ressources[1].device_number = 0;
  c->speaking_ressources[2].device_number = 0;
  c->speaking_ressources[3].device_number = 0;
  c->speaking_ressources[4].device_number = 0;

  

  return 0;
};

void Radio::init_mixers() // virtual
{
	if ( has_hardware_error() ) return;
	//radio_port_set_initial_mixer_config(&port_aga_reception); 
	//radio_port_set_initial_mixer_config(&port_aga_emission);

	{
		vega_port_t *p = &port_aga_reception;
		mixer_disable_sn(p, 3); 	/* si enable on recoit le tone sur les postes CRM*/
		mixer_enable_sn(p, 1);
		mixer_disable_sn(p, 2);

	}

	{
		vega_port_t *p = &port_aga_emission;
		mixer_disable_sn(p, 3);
		mixer_enable_sn(p, 1);
		mixer_disable_sn(p, 2);

		/* connecte le timeslot de valeur device number - device number de depart des postes radios */
		//ts_value = FIRST_DEVICE_RADIO_INPUT_TS /*121*/ + ((device_t*)p->my_device)->number - FIRST_RADIO_DEVICE_NUMBER; /* shift les postes radio commencent a 25 ! */
		vega_log(INFO, "configure initial mixer VEGA_PORT_RADIO_EMISSION TS%03d", m_timeslot_micro);
		mixer_connect_timeslot_to_input(p, m_timeslot_micro); /* lien marron : exemple radio vers ts 111 */
	}
}

void CRM4::init_mixers()
{  
	if ( has_hardware_error() ) return;
	// pdl 20090305 :port_3confs a faire en premier sinon l'action d'apres plante
	//if ( false==init_mixer_done )
	{
		init_mixer_done = true;
		
		{	// FIRST_DEVICE_CRM4_3CONF_MIXER_TS
			vega_port_t *p = &port_3confs;
			int ts_value;
			mixer_enable_sn(p, 3);
			mixer_enable_sn(p, 1);
			mixer_disable_sn(p, 2);
			vega_log(INFO, "configure initial mixer@%p as vega_port_mixer_three_confs", &p->mixer);

			ts_value = FIRST_DEVICE_CRM4_3CONF_MIXER_TS; // 168
			ts_value += port_aslt.port_number;//port_number; // ATTENTION: No du port ASLT
			ts_value += (port_aslt.board_number - 1) * 6;
			/* A FAIRE : commencer par 19, puis 18 puis 17 => ca decremente  ts_value += (get_board_number_from_address(p->address) - 1) * 6; */
			mixer_connect_timeslot_to_input(p, ts_value); 
		}

		{	// FIRST_DEVICE_CRM4_INPUT_TS
			vega_port_t *p = &port_aslt;
			int ts_value;
			mixer_enable_sn(p, 3);
			mixer_disable_sn(p, 1);
			mixer_disable_sn(p, 2);

			/* en fonction du numero du port, on va le connecter sur des ts qui vont bien */
			ts_value = FIRST_DEVICE_CRM4_INPUT_TS; // 144
			ts_value += port_aslt.port_number;//port_number;
			ts_value += (port_aslt.board_number - 1) * 6;

			mixer_connect_timeslot_to_input(p , ts_value);


			/* FIRST_DEVICE_CRM4_3CONF_MIXER_TS    connexion ts mixer 3 confs */
			ts_value = FIRST_DEVICE_CRM4_3CONF_MIXER_TS; // 168
			ts_value += port_aslt.port_number;//port_number;
			ts_value += (port_aslt.board_number - 1) * 6;

			// test with Hans 20090319
			mixer_connect_timeslot_to_sub_output(p, sub_channel_3, ts_value, 1);
		}
	}

	/* active les events sur les devices */
	device_enable_events();
	/* crm4 avec device */
	device_set_option_display_present();
	device_set_backlight_on();
	/* on charge la configuration des touches */

	// todo : device_send_side_tone_off() [138 , addr port]
	/* libre et dans aucune configuration */
	//device_change_state(d, state_free);

	// ===> pdl 20090313
	device_side_tone_off();

	device_line_printf(1,"D%d:%s                        ",number,name);
	device_line_printf(2,".............................");
	device_line_printf(3,".............................");
	//d->device_line_printf(4,".............................");
	device_line_print_time_now(4);

	init_all_leds();//d->device_reset_all_leds();

}

//static int vega_port_set_initial_mixer_config(vega_port_t *p)
int vega_conference_t::conf_vega_port_set_initial_mixer_config(vega_port_t *p)
{
  int ts_value;

  /* en fonction du type de port , on va effectuer la configuraiton initiale du port */
  switch (p->type) 
  {


  case VEGA_PORT_AGA_MIX_JUP2_RADIO1_RADIO2:
    vega_log(INFO, "configure initial mixer@%p as vega_port_aga_mix_jup2_radio1_radio2", &p->mixer);
    
    /* initialise les SN */
    mixer_enable_sn(p, 3);
    mixer_disable_sn(p, 1);
    mixer_disable_sn(p, 2);
    /* configurer l'entrée sur le ts_start de la conf */
    ts_value = p->conference_matrix_configuration->ts_start; /* 31 pour la 1ere conf */
    mixer_connect_timeslot_to_input(p, ts_value);

    break;
  case VEGA_PORT_AGA_MIX_DIRECTOR_SECONDARY1_SECONDARY2:
    vega_log(INFO, "configure initial mixer@%p as vega_port_aga_mix_director_secondary1_secondary2", &p->mixer);
    
    mixer_enable_sn(p, 3);
    mixer_disable_sn(p, 1);
    mixer_disable_sn(p, 2);

    ts_value = p->conference_matrix_configuration->ts_start + 1;
    mixer_connect_timeslot_to_input(p, ts_value); /* 32 pour la 1ere conf */
    break;

   case VEGA_PORT_AGA_RADIO1:
    {

      vega_log(INFO, "configure initial mixer@%p as VEGA_PORT_AGA_RADIO1", &p->mixer);
      
      mixer_enable_sn(p, 3);
      mixer_disable_sn(p, 1);
      mixer_disable_sn(p, 2);
      
      /* connecte la sous sortie 1 vers le timeslot 32 (pour la 1ere conf) */
      ts_value = p->conference_matrix_configuration->ts_start + 1;
      mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);

      /* connecte l'entrée */
      ts_value = p->conference_matrix_configuration->ts_start + 2;      
      mixer_connect_timeslot_to_input(p, ts_value ); /* 33 pour la 1ere conf */
      
      
    }
    break;


  case VEGA_PORT_AGA_RADIO2:
    {

      vega_log(INFO, "configure initial mixer@%p as vega_port_aga_radio2", &p->mixer);
      
      mixer_enable_sn(p, 3);
      mixer_disable_sn(p, 1);
      mixer_disable_sn(p, 2);

      /* connecte la sous sortie 1 vers le timeslot 32 (pour la 1ere conf) */
      ts_value = p->conference_matrix_configuration->ts_start + 1;
      mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);
      /* connecte l'entrée */
      ts_value = p->conference_matrix_configuration->ts_start + 3;
      
      mixer_connect_timeslot_to_input(p, ts_value ); /* 34 pour la 1ere conf */
      
      
    }
    break;

  case VEGA_PORT_AGA_JUPITER2:
    {

      vega_log(INFO, "configure initial mixer@%p as VEGA_PORT_AGA_JUPITER2", &p->mixer);
      
      mixer_enable_sn(p, 3);
      mixer_disable_sn(p, 1);
      mixer_disable_sn(p, 2);
      
      /* connecte la sous sortie 1 vers le timeslot 32 (pour la 1ere conf) */
      ts_value = p->conference_matrix_configuration->ts_start + 1;
      mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);

      /* connecte l'entrée */
      ts_value = p->conference_matrix_configuration->ts_start + 4;      
      mixer_connect_timeslot_to_input(p, ts_value ); /* 36 pour la 1ere conf */
      
      
    }
    break;

  case VEGA_PORT_AGA_DIRECTOR:
    vega_log(INFO, "configure initial mixer@%p as vega_port_aga_director", &p->mixer);
    
    mixer_enable_sn(p, 3);
    mixer_disable_sn(p, 1);
    mixer_disable_sn(p, 2);

    ts_value = p->conference_matrix_configuration->ts_start ;
    mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);

    /* connecte l'entrée */
    ts_value = p->conference_matrix_configuration->ts_start + 5;
    mixer_connect_timeslot_to_input(p, ts_value ); /* 36 pour la 1ere conf */

    break;
  case VEGA_PORT_AGA_SECONDARY1:
    vega_log(INFO, "configure initial mixer@%p as vega_port_aga_secondary1", &p->mixer );
    
    mixer_enable_sn(p, 3);
    mixer_disable_sn(p, 1);
    mixer_disable_sn(p, 2);

    ts_value = p->conference_matrix_configuration->ts_start;
    mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);
    /* connecte l'entrée */
    ts_value = p->conference_matrix_configuration->ts_start + 6;
    mixer_connect_timeslot_to_input(p, ts_value ); /* 37 pour la 1ere conf */

    break;

  case VEGA_PORT_AGA_SECONDARY2:
    vega_log(INFO, "configure initial mixer@%p as vega_port_aga_secondary2", &p->mixer);
    
    mixer_enable_sn(p, 3);
    mixer_disable_sn(p, 1);
    mixer_disable_sn(p, 2);

    ts_value = p->conference_matrix_configuration->ts_start;
    mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);
    /* connecte l'entrée */
    ts_value = p->conference_matrix_configuration->ts_start + 7;
    mixer_connect_timeslot_to_input(p, ts_value ); /* 38 pour la 1ere conf */

    break;
    
  case VEGA_PORT_AGA_MIX_CONF:
    
    mixer_enable_sn(p, 3);
    /* ATTENTION : ici S/N 1 active pour les recorder */
    mixer_enable_sn(p, 1);
    mixer_disable_sn(p, 2);

    ts_value = p->conference_matrix_configuration->ts_start ; /* cnx mixer radio  + jupiter */
    mixer_connect_timeslot_to_sub_output(p, sub_channel_0, ts_value, 1);

    ts_value = p->conference_matrix_configuration->ts_start + 1; /* cnx mixer postes */
    mixer_connect_timeslot_to_sub_output(p, sub_channel_1, ts_value, 1);

    /* connecte l'entree */
    ts_value = p->conference_matrix_configuration->ts_start + 8;
    mixer_connect_timeslot_to_input(p, ts_value ); /* 39 pour la 1ere conf ; remplace jupiter 2*/
    vega_log(INFO, "config VEGA_PORT_AGA_MIX_CONF TS %d", ts_value);

    break;

  default:
    break;
  }
  return 0;
}

#if 0

//#define FIRST_DEVICE_JUP2_INPUT_TS			137  /*  137 -> 142 */
//#define FIRST_DEVICE_RADIO_INPUT_TS			121  /*  121 -> 136 */
//#define FIRST_RADIO_DEVICE_NUMBER		25 // D25 == VHF1
//#define FIRST_JUP2_DEVICE_NUMBER		41 // D41 == JUPITER1
int Radio::radio_port_set_initial_mixer_config(vega_port_t *p)
{
  switch (p->type) 
  {

  case VEGA_PORT_RADIO_EMISSION: /* 1er port de la paire de port */
    {
      //int ts_value = ((device_t*)p->my_device)->m_timeslot_micro;

      /* CA MARCHE !!! tests ok: postes crm entendent bien la radio 1 */
      
      /* initialise les SN */
      mixer_disable_sn(p, 3);
      mixer_enable_sn(p, 1);
      mixer_disable_sn(p, 2);

      /* connecte le timeslot de valeur device number - device number de depart des postes radios */
      //ts_value = FIRST_DEVICE_RADIO_INPUT_TS /*121*/ + ((device_t*)p->my_device)->number - FIRST_RADIO_DEVICE_NUMBER; /* shift les postes radio commencent a 25 ! */


      vega_log(INFO, "configure initial mixer VEGA_PORT_RADIO_EMISSION TS%03d", m_timeslot_micro);

      mixer_connect_timeslot_to_input(p, m_timeslot_micro); /* lien marron : exemple radio vers ts 111 */
      
    }
    break;

  case VEGA_PORT_JUP2_EMISSION: /* 1er port de la paire de port */
    {
      int ts_value = 0;
      /* CA MARCHE ??? postes crm entendent bien le jup2 ??? */
      
      /* initialise les SN */
      mixer_disable_sn(p, 3);
      mixer_enable_sn(p, 1);
      mixer_disable_sn(p, 2);

      /* connecte le timeslot de valeur device number - device number de depart des postes radios */
      //ts_value = FIRST_DEVICE_JUP2_INPUT_TS /*137*/ + 
		//  ((device_t*)p->my_device)->number - FIRST_JUP2_DEVICE_NUMBER; /* shift les postes jup2 commencent a 41 ! */

      vega_log(INFO, "configure initial mixer VEGA_PORT_JUP2_EMISSION TS%03d", m_timeslot_micro);

      mixer_connect_timeslot_to_input(p, m_timeslot_micro); /* lien marron : exemple jupiter2 vers ts 137 */
      
    }
    break;

  case VEGA_PORT_JUP2_RECEPTION:
  case VEGA_PORT_RADIO_RECEPTION:
    {
      /* 2eme port de la paire de port : port en sortie (mix conf + appel general)*/
      
      mixer_disable_sn(p, 3); 	/* si enable on recoit le tone sur les postes CRM*/
      mixer_enable_sn(p, 1);
      mixer_disable_sn(p, 2);
    }
    break;


  }
}

#endif
