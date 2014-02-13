/*
 * satip: startup and main loop
 *
 * Copyright (C) 2014  mc.fishdish@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include "satip_config.h"
#include "satip_vtuner.h"
#include "satip_rtsp.h"
#include "satip_rtp.h"
#include "log.h"
#include "polltimer.h"



int dbg_level = MSG_DEBUG;
unsigned int dbg_mask = MSG_ALL; 
int use_syslog = 0;

static int cseq=0;

void test_sequencer(int param)
{
  t_satip_config* sc=(t_satip_config*)param;

  cseq++;
  
  switch (cseq)
    {
    case 1:
      satip_set_freq(sc,118360);
      satip_set_modsys(sc, SATIPCFG_MS_DVB_S);
      satip_set_pilots(sc, SATIPCFG_P_OFF);
      satip_set_fecinner(sc, SATIPCFG_F_34);
      satip_set_polarization(sc, SATIPCFG_P_HORIZONTAL);
      satip_set_rolloff(sc, SATIPCFG_R_0_35);
      satip_set_symbol_rate(sc,27500);
      satip_set_modtype(sc, SATIPCFG_MT_QPSK);
      satip_add_pid(sc, 18);
      break;

    case 200:
      //satip_add_pid(sc, 19);
      break;

    case 400:
      break;

    default: 
      break;
      
    }
}


int main(int argc, char** argv)
{
  char* host = NULL;
  char* port = "554";

  t_satip_config* satconf;
  struct satip_rtsp* srtsp;
  struct satip_rtp* srtp;
  struct satip_vtuner* satvt;

  struct pollfd pollfds[2];
  struct polltimer* timerq=NULL;

  char opt;

  while((opt = getopt(argc, argv, "h:p")) != -1 ) {
    switch(opt) 
      {
      case 'h': 
	host = optarg;
	break;
	
      case 'p': 
	port = optarg;
	break;
	
      default:
        ERROR(MSG_MAIN,"unknown option %c\n",opt);
	exit(1);	
      }
  }

  if ( host==NULL ) {
    ERROR(MSG_MAIN,"No host argument\n");
    exit(1);
  }
      


  satconf = satip_new_config();

#if 0
  {

    struct polltimer_periodic* periodic;
    polltimer_periodic_start(&timerq, 
			   &periodic,
			   test_sequencer,
			   10,
			   (int)satconf);
  }
#endif


#if 1
  satvt = satip_vtuner_new("/dev/vtunerc0", satconf);

  srtp  = satip_rtp_new(satip_vtuner_fd(satvt));

#else
  srtp  = satip_rtp_new(0);

#endif

  srtsp = satip_rtsp_new(satconf,&timerq, host, port,
			 satip_rtp_port(srtp));
  
  pollfds[0].fd=satip_vtuner_fd(satvt);
  pollfds[0].events = POLLPRI;
  

  while (1)
    {
      /* apply any updates on rtsp  */
      satip_rtsp_check_update(srtsp);

      /* vt control events */
      pollfds[0].revents = 0;
      
      /* rtsp socket may be closed */
      pollfds[1].fd = satip_rtsp_socket(srtsp);
      pollfds[1].events = satip_rtsp_pollflags(srtsp);
      pollfds[1].revents = 0;
      
      /* poll and timeout on next pending timer */
      if ( poll(pollfds, pollfds[1].events == 0 ? 1 : 2, 
		polltimer_next_ms(timerq) ) ==-1 && 
	   errno!=EINTR )
	{
	  perror(NULL);
	  exit(1);
	}

      /* schedule timer callbacks */
      polltimer_call_next(&timerq);
	
      /* vt control event handling */
      if ( pollfds[0].revents !=0 )
	satip_vtuner_event(satvt);

      /* rtsp event handling */
      if ( pollfds[1].revents !=0 )
	satip_rtsp_pollevents(srtsp, pollfds[1].revents);      
    }
  
  return 0;
  
}

