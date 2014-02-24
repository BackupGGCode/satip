/*
 * satip: vtuner to satip mapping
 *
 * Copyright (C) 2014  mc.fishdish@gmail.com
 * [fragments from vtuner by Honza Petrous]
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

#include "satip_config.h"
#include "satip_vtuner.h"
#include "log.h"

/* from vtunerc_priv.h */
#define MAX_PIDTAB_LEN 30
#define PID_UNKNOWN 0x0FFFF

/* fixme: align with driver */
typedef unsigned int   u32;
typedef signed int     s32;
typedef unsigned short u16;
typedef unsigned char  u8;

/* driver interface */
#include "vtuner.h"



typedef struct satip_vtuner {
  int fd;
  int tone;
  char tone_set;
  t_satip_config* satip_cfg;
} t_satip_vtuner;




/* according to frontend.h */
static t_fec_inner fec_map[] ={ 
  SATIPCFG_F_NONE,   // FEC_NONE = 0,
  SATIPCFG_F_12,     // FEC_1_2
  SATIPCFG_F_23,     // FEC_2_3
  SATIPCFG_F_34,     // FEC_3_4
  SATIPCFG_F_45,     // FEC_4_5
  SATIPCFG_F_56,     // FEC_5_6
  SATIPCFG_F_NONE,   // FEC_6_7
  SATIPCFG_F_78,     // FEC_7_8
  SATIPCFG_F_89,     // FEC_8_9
  SATIPCFG_F_NONE,   // FEC_AUTO
  SATIPCFG_F_35,     // FEC_3_5
  SATIPCFG_F_910,    // FEC_9_10
  SATIPCFG_F_NONE    // FEC_2_5
};



struct dvb_frontend_info fe_dvbs2 = {
  .name                  = "vTuner DVB-S2",
  .type                  = 0,
  .frequency_min         = 925000,
  .frequency_max         = 2175000,
  .frequency_stepsize    = 125000,
  .frequency_tolerance   = 0,
  .symbol_rate_min       = 1000000,
  .symbol_rate_max       = 45000000,
  .symbol_rate_tolerance = 0,
  .notifier_delay        = 0,
  .caps                  = FE_CAN_INVERSION_AUTO | 
  FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 | FE_CAN_FEC_4_5 | 
  FE_CAN_FEC_5_6 | FE_CAN_FEC_7_8 | FE_CAN_FEC_8_9 |
  FE_CAN_QPSK | FE_CAN_RECOVER
};



t_satip_vtuner* satip_vtuner_new(char* devname,t_satip_config* satip_cfg)
{
  int fd;
  t_satip_vtuner* vt;

  fd  = open(devname, O_RDWR);
  if ( fd < 0)
    {
      ERROR(MSG_MAIN,"Couldn't open %s\n",devname);
      return NULL;
    }
  
  if ( ioctl(fd, VTUNER_SET_NAME, "vTuner")      ||
       ioctl(fd, VTUNER_SET_TYPE, "DVB-S2")      || 
       ioctl(fd, VTUNER_SET_FE_INFO, &fe_dvbs2)  )
    return NULL;
  
  vt=(t_satip_vtuner*)malloc(sizeof(t_satip_vtuner));
  
  vt->fd=fd;
  vt->satip_cfg=satip_cfg;
  
  /* set default position A, if appl. does not configure */
  satip_set_position(satip_cfg,1);

  vt->tone_set=0;

  return vt;
}

int satip_vtuner_fd(struct satip_vtuner* vt)
{
  return vt->fd;
}


static void set_frontend(struct satip_vtuner* vt, struct vtuner_message* msg)
{
  int fec_inner;
  int frequency = msg->body.fe_params.frequency/100;

  if ( !vt->tone_set ) 
    {
      DEBUG(MSG_MAIN,"cannot tune: no band selected\n");
      return;
    } 

  /* revert frequency shift */
  if ( vt->tone == SEC_TONE_ON ) /* high band */
    frequency += 106000;
  else /* low band */
    if ( frequency-97500 < 0 )
      frequency+=97500;
    else
      frequency-=97500;

  satip_set_freq(vt->satip_cfg, frequency);

  /* symbol rate */
  satip_set_symbol_rate(vt->satip_cfg, msg->body.fe_params.u.qpsk.symbol_rate/1000 );
  
  /* modulation system and modulation piggybacked in FEC */
  fec_inner=msg->body.fe_params.u.qpsk.fec_inner;

  if ( ( fec_inner & 31 ) < SATIPCFG_F_NONE )
    satip_set_fecinner(vt->satip_cfg, fec_map[fec_inner & 31]);
  else
    ERROR(MSG_MAIN,"invalid FEC configured\n");

  if ( fec_inner & 32 )
    {
      int inversion=msg->body.fe_params.inversion;

      satip_set_modsys(vt->satip_cfg, SATIPCFG_MS_DVB_S2);

      /* DVB-S2: modulation */
      if ( fec_inner & 64 )
	satip_set_modtype(vt->satip_cfg, SATIPCFG_MT_8PSK);
      else 
	satip_set_modtype(vt->satip_cfg, SATIPCFG_MT_QPSK);

      /* DVB-S2: rolloff and pilots encoded in inversion */     
      if ( inversion & 0x04 )
	satip_set_rolloff(vt->satip_cfg,SATIPCFG_R_0_25);
      else if (inversion & 0x08)
	satip_set_rolloff(vt->satip_cfg,SATIPCFG_R_0_20);
      else
	satip_set_rolloff(vt->satip_cfg,SATIPCFG_R_0_35);

      if ( inversion & 0x10 )
	satip_set_pilots(vt->satip_cfg,SATIPCFG_P_ON);
      else if ( inversion & 0x20 )
	satip_set_pilots(vt->satip_cfg,SATIPCFG_P_ON);
      else 
	satip_set_pilots(vt->satip_cfg,SATIPCFG_P_OFF);
    }
  else 
    satip_set_modsys(vt->satip_cfg, SATIPCFG_MS_DVB_S);


  DEBUG(MSG_MAIN,"MSG_SET_FRONTEND freq: %d symrate: %d \n",
	 frequency,
	 msg->body.fe_params.u.qpsk.symbol_rate );
}

static void set_tone(struct satip_vtuner* vt, struct vtuner_message* msg)
{
  vt->tone = msg->body.tone;
  vt->tone_set = 1;

  DEBUG(MSG_MAIN,"MSG_SET_TONE:  %s\n",vt->tone == SEC_TONE_ON  ? "ON = high band" : "OFF = low band");  
}


static void set_voltage(struct satip_vtuner* vt, struct vtuner_message* msg)
{
  if ( msg->body.voltage == SEC_VOLTAGE_13 )
    satip_set_polarization(vt->satip_cfg, SATIPCFG_P_VERTICAL);
  else if (msg->body.voltage == SEC_VOLTAGE_18)
    satip_set_polarization(vt->satip_cfg, SATIPCFG_P_HORIZONTAL);
  else  /*  SEC_VOLTAGE_OFF */
    satip_clear_config(vt->satip_cfg);
  
  DEBUG(MSG_MAIN,"MSG_SET_VOLTAGE:  %d\n",msg->body.voltage);
}


static void diseqc_msg(struct satip_vtuner* vt, struct vtuner_message* msg)
{
  char dbg[50];
  struct diseqc_master_cmd* cmd=&msg->body.diseqc_master_cmd;
  
  if ( cmd->msg[0] == 0xe0 &&
       cmd->msg[1] == 0x10 &&
       cmd->msg[2] == 0x38 &&
       cmd->msg_len == 4 )
    {
      /* committed switch */
      u8 data=cmd->msg[3];

      if ( (data & 0x01) == 0x01 )
	{
	  vt->tone = SEC_TONE_ON; /* high band */
	  vt->tone_set=1;
	}
      else if ( (data & 0x11) == 0x10 )
	{
	  vt->tone = SEC_TONE_OFF; /* low band */
	  vt->tone_set=1;
	}

      if ( (data & 0x02) == 0x02 )
	satip_set_polarization(vt->satip_cfg, SATIPCFG_P_HORIZONTAL);	
      else if ( (data & 0x22) == 0x20 )
	satip_set_polarization(vt->satip_cfg, SATIPCFG_P_VERTICAL);

      /* some invalid combinations ? */
      satip_set_position(vt->satip_cfg, ( (data & 0x0c) >> 2) + 1 );
    }
  
  sprintf(dbg,"%02x %02x %02x   msg %02x %02x %02x len %d",
	  msg->body.diseqc_master_cmd.msg[0],
	  msg->body.diseqc_master_cmd.msg[1],
	  msg->body.diseqc_master_cmd.msg[2],
	  msg->body.diseqc_master_cmd.msg[3],
	  msg->body.diseqc_master_cmd.msg[4],
	  msg->body.diseqc_master_cmd.msg[5],
	  msg->body.diseqc_master_cmd.msg_len);
  DEBUG(MSG_MAIN,"MSG_SEND_DISEQC_MSG:  %s\n",dbg);
}      



static void set_pidlist(struct satip_vtuner* vt, struct vtuner_message* msg)
{
  int i;
  u16* pidlist=msg->body.pidlist;

  DEBUG(MSG_MAIN,"MSG_SET_PIDLIST:\n");
  
  satip_del_allpid(vt->satip_cfg);

  for (i=0; i<MAX_PIDTAB_LEN; i++)
    if (pidlist[i] < 8192  )
      {
	satip_add_pid(vt->satip_cfg,pidlist[i]);
	DEBUG(MSG_MAIN,"%d\n",pidlist[i]);
      }
}





void satip_vtuner_event(struct satip_vtuner* vt)
{
  struct vtuner_message  msg;

  if (ioctl(vt->fd, VTUNER_GET_MESSAGE, &msg)) 
    return;

  switch(msg.type)
    {
    case MSG_SET_FRONTEND:
      set_frontend(vt,&msg);
      break;

    case MSG_SET_TONE:
      set_tone(vt,&msg);
      break;

    case MSG_SET_VOLTAGE:
      set_voltage(vt,&msg);
      break;
    
    case MSG_PIDLIST:
      set_pidlist(vt,&msg);
      DEBUG(MSG_MAIN,"set_pidlist: no response required\n");
      return;
      break;
      
    case MSG_READ_STATUS:  
      DEBUG(MSG_MAIN,"MSG_READ_STATUS\n");
      msg.body.status =    // tuning ok!
	// FE_HAS_SIGNAL     |
	// FE_HAS_CARRIER    |
	// FE_HAS_VITERBI    |
	// FE_HAS_SYNC       |
	FE_HAS_LOCK;
      break;

    case MSG_READ_BER:
      DEBUG(MSG_MAIN,"MSG_READ_BER\n");
      msg.body.ber = 0;
      break;

    case MSG_READ_SIGNAL_STRENGTH:
      DEBUG(MSG_MAIN,"MSG_READ_SIGNAL_STRENGTH\n");
      msg.body.ss = 50;
      break;
      
    case MSG_READ_SNR:
      DEBUG(MSG_MAIN,"MSG_READ_SNR\n");
      msg.body.snr = 900; /* ?*/
      break;
    
    case MSG_READ_UCBLOCKS:
      DEBUG(MSG_MAIN,"MSG_READ_UCBLOCKS\n");
      msg.body.ucb = 0;
      break;

    case MSG_SEND_DISEQC_BURST:
      DEBUG(MSG_MAIN,"MSG_SEND_DISEQC_BURST\n");
      if ( msg.body.burst == SEC_MINI_A )
	satip_set_position(vt->satip_cfg,1);
      else if ( msg.body.burst == SEC_MINI_B )
	satip_set_position(vt->satip_cfg,2);
      else
	ERROR(MSG_MAIN,"invalid diseqc burst %d\n",msg.body.burst);
      break;

    case MSG_SEND_DISEQC_MSG:
      diseqc_msg(vt, &msg);
      break;
      
    default:
      break;
    }
  
  msg.type=0;

  if (ioctl(vt->fd, VTUNER_SET_RESPONSE, &msg)) 
    return;

  DEBUG(MSG_MAIN,"ioctl: response ok\n");
}
