// const.h
#ifndef _CONST_H_
#define _CONST_H_

#ifndef BYTE 
#define BYTE unsigned char
#endif 

#ifndef DWORD
#define DWORD unsigned long
#endif 

#ifndef WORD
#define WORD unsigned short
#endif 

#define max(a, b)	((a) > (b) ? (a) : (b))
#define min(a, b)	((a) < (b) ? (a) : (b))

#define CRM4_MAX_KEYS (4+(8*6)) // 4 touches de fonction + 10 conferences + ...

#define MAX_SIMULTANEOUS_ACTIVE_CONFERENCES		4

#define MAX_BOARD_NUMBER			25 // 25 est la carte mere alphacom !!

#define CONF_NB_AGA_PORTS			9

#define MAX_CONFERENCES				8 // defaut 6 conferences available with aga 17, 18 and 19

#define MAX_PARTICIPANTS			50
#define MAX_SPEAKER_RESSOURCES		6

#define MAX_GROUPS					20
#define MAX_DEVICE_IN_GROUP_CALL	20

#define MAX_DEVICE_NUMBER			46 // D1 .. D24 ( CRM4 ) D25... D40 ( Radio ) D41 ... D46 ( Jupiter )  
//#define MAX_CRM4_NUMBER				24
//#define MAX_RADIO_NUMBER			40
//#define MAX_JUPITER2_NUMBER			46

#define MAX_SIMULTANEOUS_ACTIVE_CONF_BY_DEVICE 4

#define DEVICES_CONF								"devices.conf"
#define CONFERENCES_CONF							"conferences.conf"
#define GROUPS_CONF									"groups.conf"
#define DEVICE_KEYS_CONF							"device-keys.conf"

#define VEGACTRL_CONF								"vegactrl.conf"

#define _ROOT_EVENT_ALARM_DIR_						"/vega/log/"
#define _ROOT_EVENT_NAME_							"evenements"
#define _ROOT_ALARM_NAME_							"alarmes"

#define _ROOT_CRITICAL_NAME_						"/tmp/critical-vegactrl"
#define _ROOT_LOG_NAME_								"/tmp/logvegactrl"


#define POLLING_REMOTE_DELAY_SECONDS 8


#define VEGA_NOT_URGENT 0
#define VEGA_URGENT 1

/* timeslot de départ pour les postes CRM4 */
#define FIRST_DEVICE_CRM4_INPUT_TS			144  /*  144 -> 167 */
#define FIRST_DEVICE_CRM4_3CONF_MIXER_TS	168  /*  168 -> 191 */
#define FIRST_DEVICE_RADIO_INPUT_TS			121  /*  121 -> 136 */
#define FIRST_DEVICE_JUP2_INPUT_TS			137  /*  137 -> 142 */

#define FIRST_RADIO_DEVICE_NUMBER		25 // D25 == VHF1
#define FIRST_JUP2_DEVICE_NUMBER		41 // D41 == JUPITER1

#define MAX_TIMESLOT 255

#define PORT_MAX_SPEAKER_RESSOURCES 5 /* 5 simultaneous CRM4 can speak in a conference when no radio , no jupiter */
#define PORT_RESERVED_INVALID (-1)

#define MILLISECOND 1000
#define MAX_TCP_FRAME_SIZE 512

#define KEEP_ALIVE_INTERVAL	10000 /* 10 sec */


/* coulours led pour les devices CRM4 */
#define LED_COLOR_RED			0x00
#define	LED_COLOR_GREEN			0x01

/* type de clignotement */
#define	LED_MODE_FIX			0x00
#define	LED_MODE_SLOW_BLINK		0X01
#define	LED_MODE_FAST_BLINK		0x02

#define MAX_MSG_SIZE 256

#define IHM_LISTENING_TCP_PORT 50000
#define IHM_LISTENING_MONITOR_RX_PORT 50001
#define IHM_LISTENING_MONITOR_TX_PORT 50002


#define DEVICE_KEY_MICRO_PRESSED		201
#define DEVICE_KEY_MICRO_RELEASED		202
#define DEVICE_KEY_CANCEL			203
#define DEVICE_UNPLUGGED			204
#define DEVICE_PLUGGED				205
#define DEVICE_NOT_PRESENT			206
#define DEVICE_PRESENT				207

// hide participants
#define UNHIDE 0
#define HIDE 1

typedef int SOCKET;
const int SOCKET_ERROR = -1;
const int INVALID_SOCKET = -1;

#define TOKENS      " ;|:"
#define TCP_RESTART "restart"
#define TCP_SUBMIT  "submit"


#define INVALID_TIMESLOT		-1
#define MAX_BOARD				24
#define AGA_PORTS_BY_BOARD		8
#define ASLT_PORTS_BY_BOARD		6
#define SBI_BY_BOARD			2
#define SUBCHANNEL_BY_PORT		3
#define MAX_PORT_BY_SBI			8

#endif
