/*
** Copyright 2022 bitValence, Inc.
** All Rights Reserved.
**
** This program is free software; you can modify and/or redistribute it
** under the terms of the GNU Affero General Public License
** as published by the Free Software Foundation; version 3 with
** attribution addendums as found in the LICENSE.txt
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Affero General Public License for more details.
**
** Purpose:
**   Manage a gateway between the cFS Software Bus and a UDP connection
**   sending JSON messages.
**
** Notes:
**   1. This app does not provide a generic message translation capability. It
**      is expected to be used in constrained environments so a trade for
**      simplicity and extendability was made over complexity and genericity.
**      A separate JSON table is defined for each supported JMSG topic combined
**      with a custom function to manage the translation.
**
*/
#ifndef _astro_pi_app_
#define _astro_pi_app_

/*
** Includes
*/

#include "app_cfg.h"
#include "astro_py.h"

/***********************/
/** Macro Definitions **/
/***********************/

/*
** Events
*/

#define ASTRO_PI_APP_INIT_APP_EID      (ASTRO_PI_APP_BASE_EID + 0)
#define ASTRO_PI_APP_NOOP_EID          (ASTRO_PI_APP_BASE_EID + 1)
#define ASTRO_PI_APP_EXIT_EID          (ASTRO_PI_APP_BASE_EID + 2)
#define ASTRO_PI_APP_INVALID_MID_EID   (ASTRO_PI_APP_BASE_EID + 3)


/**********************/
/** Type Definitions **/
/**********************/


/******************************************************************************
** Command Packets
*/
/* See EDS */

/******************************************************************************
** Telemetry Packets
*/
/* See EDS */

/******************************************************************************
** App Class
*/
typedef struct 
{

   /* 
   ** App Framework
   */ 

   INITBL_Class_t    IniTbl; 
   CFE_SB_PipeId_t   CmdPipe;
   CMDMGR_Class_t    CmdMgr;
      
   /*
   ** Telemetry Packets
   */
   
   ASTRO_PI_StatusTlm_t   StatusTlm;

   
   /*
   ** ASTRO_PI State & Contained Objects
   */ 
   
   uint32 PerfId;
   
   CFE_SB_MsgId_t  CmdMid;
   CFE_SB_MsgId_t  SendStatusMid;
   
   ASTRO_PY_Class_t AstroPy;

} ASTRO_PI_APP_Class_t;


/*******************/
/** Exported Data **/
/*******************/

extern ASTRO_PI_APP_Class_t  AstroPiApp;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: JMSG_AppMain
**
*/
void ASTRO_PI_AppMain(void);


#endif /* _astro_pi_app_ */
