/* 
project: PC control Vega Zenitel 2009
filename: groups.c
author: Mustafa Ozveren & Debreuil Philippe
creation: 20090402
desciption: group call and general call
*/
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

#include "const.h"
#include "debug.h"
#include "conf.h"
#include "misc.h"
#include "rude/config.h"

#define min(a, b)	((a) < (b) ? (a) : (b))

GROUP_T tab_appel_group[MAX_GROUPS]; 

bool CRM4::has_group_key(int grpnum) // rechercher si une touche correspond a ce groupe	
{  // dans la map keymap_groups
	 QMap<int, int>::const_iterator i = keymap_groups.constBegin();
	 while (i != keymap_groups.constEnd()) {
		 //cout << i.key() << ": " << i.value() << endl;
		 if ( i.value() == grpnum ) {
			 printf("D%d has_group_key G%d in KEY %d\n", number, grpnum , i.key());
			 return true;
		 }
		 ++i;
	 }
	 return false;
	/*QMap<int, int>::const_iterator i = keymap_groups.find(grpnum);
	if ( keymap_groups.end() != i ) {
		 fprintf(stderr,"clef du groupe G%d en position %d sur CRM4 %d", grpnum, i.value() , number)  ;
		return true;
	}
	return false;*/
}

int dump_group(FILE* fout)
{
	if ( NULL == fout ) return 0;
	int NG;
	for ( NG=1; NG< MAX_GROUPS; NG++)
	{
		fprintf(fout, "GROUP G%d : ", NG);
		GROUP_T* ptr_G = &tab_appel_group[NG];
		int i;for (i=0;i< ptr_G->nb_devices; i++){
			fprintf(fout, "%d ", ptr_G->tab_devices[i]);
		}
		fprintf(fout,"\n");
	}
	return 0;
} 

int is_valid_group_number(int NG)
{
	if ( NG>=1 && NG<MAX_GROUPS ) return 1;
	return 0;
}

int is_device_number_in_group(int devnum,int NG)
{
	if ( !is_valid_group_number(NG) ) {return 0;}

	GROUP_T* ptr_G = &tab_appel_group[NG];
	if ( NULL==ptr_G ) {return 0;}

	for ( int j=0; j<ptr_G->nb_devices ; j++){
		if  ( ptr_G->tab_devices[j] ==devnum ){
			vega_log(INFO, "YES D%d in G%d (total:%d)", devnum, NG, ptr_G->nb_devices);
			return 1;
		}
	}
	vega_log(INFO, "NO D%d in NOT in G%d (total:%d)", devnum, NG, ptr_G->nb_devices);
	return 0;
}

