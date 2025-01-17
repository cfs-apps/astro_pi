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
**   Manage Astro Pi python scripts  
**
** Notes:
**   None
**
*/

#ifndef _astro_py_
#define _astro_py_

/*
** Includes
*/

#include "app_cfg.h"

/***********************/
/** Macro Definitions **/
/***********************/


/*
** Event Message IDs
*/

#define ASTRO_PY_READ_SCRIPT_FILE_EID      (ASTRO_PY_BASE_EID + 0)
#define ASTRO_PY_SEND_SCRIPT_CMD_EID       (ASTRO_PY_BASE_EID + 1)
#define ASTRO_PY_SEND_TEST_SCRIPT_CMD_EID  (ASTRO_PY_BASE_EID + 2)


/**********************/
/** Type Definitions **/
/**********************/

typedef struct
{
   
   uint32   SentScriptCnt;
   char     LastSentScript[OS_MAX_PATH_LEN];

} ASTRO_PY_Class_t;


/************************/
/** Exported Functions **/
/************************/


/******************************************************************************
** Function: ASTRO_PY_Constructor
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void ASTRO_PY_Constructor(ASTRO_PY_Class_t *UdpMgrPtr, uint32 PyScriptTlmTopicId);


/******************************************************************************
** Function: ASTRO_PY_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void ASTRO_PY_ResetStatus(void);


/******************************************************************************
** Function: ASTRO_PY_SendScriptCmd
**
*/
bool ASTRO_PY_SendScriptCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


/******************************************************************************
** Function: ASTRO_PY_SendTestScriptCmd
**
*/
bool ASTRO_PY_SendTestScriptCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr);


#endif /* _astro_py_ */
