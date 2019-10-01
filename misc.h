#ifndef __MISC_H__
#define __MISC_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include "messages.h"
#include "conf.h"

int enable_radio_dry_contact();
int dos_kbhit (void);
int install_options (void);
void restaure_options (void);
int set_radio_dry_contact_on_off(int radio_device_number , int on_off) ;
DWORD GetPrivateProfileString(const char* section,const char* lpKeyName,const char* lpDefault, char* lpReturnedString, DWORD nSize, const char* filename);
void readline(FILE*fi, char*line, int maxlen, int bCutOffComments) ;
double time_between_button(struct timeval* ptv);
const char *strncpy_only_digit_az_AZ (char *dest, const char *src, size_t n /*taille maxi de dest */ );

#endif
