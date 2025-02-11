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
**   Manage sending python scripts to the Astro Pi  
**
** Notes:
**   None
**
*/

#ifndef _py_script_
#define _py_script_

/*
** Includes
*/

#include "app_cfg.h"
#include "jmsg_platform_eds_defines.h"
#include "jmsg_lib_eds_interface.h"

/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define PY_SCRIPT_READ_FILE_EID         (PY_SCRIPT_BASE_EID + 0)
#define PY_SCRIPT_SEND_LOCAL_CMD_EID    (PY_SCRIPT_BASE_EID + 1)
#define PY_SCRIPT_SEND_TEST_CMD_EID     (PY_SCRIPT_BASE_EID + 2)
#define PY_SCRIPT_START_REMOTE_CMD_EID  (PY_SCRIPT_BASE_EID + 3)
#define PY_SCRIPT_CREATE_SENSE_HAT_EID  (PY_SCRIPT_BASE_EID + 4)


/**********************/
/** Type Definitions **/
/**********************/


typedef struct
{
   
   JMSG_LIB_TopicScriptCmd_t  TopicScriptCmd;
   ASTRO_PI_SenseHatTlm_t     SenseHatTlm;
   
   uint32   SentCnt;
   char     LastSent[OS_MAX_PATH_LEN];


} PY_SCRIPT_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: PY_SCRIPT_Constructor
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void PY_SCRIPT_Constructor(PY_SCRIPT_Class_t *PyScriptPtr, uint32 TopicScriptCmdTopicId, uint32 SenseHatTlmTopicId);


/******************************************************************************
** Function: PY_SCRIPT_CreateSenseHatTlm
**
** Notes:
**   1. Loads Sense Hat telemetry parameters fields from the JMsg and sends
**      the Sense Hat message. The order of parameters must match the
**      ASTRO_PI_SenseHatTlmParams_Enum_t definition.
**
*/
bool PY_SCRIPT_CreateSenseHatTlm(const CFE_MSG_Message_t *JMsgScriptTlm);


/******************************************************************************
** Function: PY_SCRIPT_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void PY_SCRIPT_ResetStatus(void);


/******************************************************************************
** Function: PY_SCRIPT_SendLocalCmd
**
*/
bool PY_SCRIPT_SendLocalCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: PY_SCRIPT_SendTestCmd
**
*/
bool PY_SCRIPT_SendTestCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: PY_SCRIPT_StartRemoteCmd
**
*/
bool PY_SCRIPT_StartRemoteCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _py_script_ */
