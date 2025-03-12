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
**   Provide an interface to an "Astro Pi"
**
** Notes:
**   None
**
*/

/*
** Includes
*/

#include <string.h>
#include "astro_pi_app.h"
#include "astro_pi_eds_cc.h"

/***********************/
/** Macro Definitions **/
/***********************/

/* Convenience macros */
#define  INITBL_OBJ      (&(AstroPiApp.IniTbl))
#define  CMDMGR_OBJ      (&(AstroPiApp.CmdMgr))
#define  PY_SCRIPT_OBJ   (&(AstroPiApp.PyScript))

/*******************************/
/** Local Function Prototypes **/
/*******************************/

static int32 InitApp(void);
static int32 ProcessCommands(void);
static void SendStatusPkt(void);


/**********************/
/** File Global Data **/
/**********************/

/* 
** Must match DECLARE ENUM() declaration in app_cfg.h
** Defines "static INILIB_CfgEnum IniCfgEnum"
*/
DEFINE_ENUM(Config,APP_CONFIG)  

static CFE_EVS_BinFilter_t  EventFilters[] =
{  
   /* Event ID                           Mask */
   {PKTUTIL_CSV_PARSE_ERR_EID,      CFE_EVS_FIRST_4_STOP},
   {PY_SCRIPT_CREATE_SENSE_HAT_EID, CFE_EVS_FIRST_4_STOP}
   
};

/*****************/
/** Global Data **/
/*****************/

ASTRO_PI_APP_Class_t   AstroPiApp;


/******************************************************************************
** Function: ASTRO_PI_AppMain
**
*/
void ASTRO_PI_AppMain(void)
{

   uint32 RunStatus = CFE_ES_RunStatus_APP_ERROR;
   
   CFE_EVS_Register(EventFilters, sizeof(EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                    CFE_EVS_EventFilter_BINARY);

   if (InitApp() == CFE_SUCCESS)      /* Performs initial CFE_ES_PerfLogEntry() call */
   {
      RunStatus = CFE_ES_RunStatus_APP_RUN; 
   }
   
   /*
   ** Main process loop
   */
   while (CFE_ES_RunLoop(&RunStatus))
   {
      
      /*
      ** The Rx and Tx child tasks manage translating and transferring the JSON
      ** messages. This loop only needs to service commands.
      */ 
      
      RunStatus = ProcessCommands();
      
   } /* End CFE_ES_RunLoop */

   CFE_ES_WriteToSysLog("Astro Pi App terminating, run status = 0x%08X\n", RunStatus);   /* Use SysLog, events may not be working */

   CFE_EVS_SendEvent(ASTRO_PI_APP_EXIT_EID, CFE_EVS_EventType_CRITICAL, "Astro Pi App terminating, run status = 0x%08X", RunStatus);

   CFE_ES_ExitApp(RunStatus);  /* Let cFE kill the task (and any child tasks) */


} /* End of ASTRO_PI_AppMain() */



/******************************************************************************
** Function: ASTRO_PI_APP_NoOpCmd
**
*/

bool ASTRO_PI_APP_NoOpCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_SendEvent (ASTRO_PI_APP_NOOP_EID, CFE_EVS_EventType_INFORMATION,
                      "No operation command received for Astro Pi App version %d.%d.%d",
                      ASTRO_PI_APP_MAJOR_VER, ASTRO_PI_APP_MINOR_VER, ASTRO_PI_APP_PLATFORM_REV);

   return true;


} /* End ASTRO_PI_APP_NoOpCmd() */


/******************************************************************************
** Function: ASTRO_PI_APP_ResetAppCmd
**
*/

bool ASTRO_PI_APP_ResetAppCmd(void *ObjDataPtr, const CFE_MSG_Message_t *MsgPtr)
{

   CFE_EVS_ResetAllFilters();

   CMDMGR_ResetStatus(CMDMGR_OBJ);
   
   PY_SCRIPT_ResetStatus();
	  
   return true;

} /* End ASTRO_PI_APP_ResetAppCmd() */


/******************************************************************************
** Function: InitApp
**
*/
static int32 InitApp(void)
{

   int32 RetStatus = APP_C_FW_CFS_ERROR;
      
      
   /*
   ** Read JSON INI Table & class variable defaults defined in JSON  
   */

   if (INITBL_Constructor(INITBL_OBJ, ASTRO_PI_APP_INI_FILENAME, &IniCfgEnum))
   {
   
      AstroPiApp.PerfId = INITBL_GetIntConfig(INITBL_OBJ, CFG_APP_MAIN_PERF_ID);
      CFE_ES_PerfLogEntry(AstroPiApp.PerfId);

      PY_SCRIPT_Constructor(PY_SCRIPT_OBJ, INITBL_GetIntConfig(INITBL_OBJ, CFG_JMSG_LIB_TOPIC_SCRIPT_CMD_TOPICID),
                            INITBL_GetIntConfig(INITBL_OBJ, CFG_ASTRO_PI_SENSE_HAT_TLM_TOPICID));

      AstroPiApp.CmdMid             = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_ASTRO_PI_CMD_TOPICID));
      AstroPiApp.SendStatusMid      = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_SEND_STATUS_TLM_TOPICID));
      AstroPiApp.JmsgTopicCsvTlmMid = CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, JMSG_LIB_TOPIC_CSV_TLM_TOPICID));
      
      /*
      ** Initialize app level interfaces
      */
 
      CFE_SB_CreatePipe(&AstroPiApp.CmdPipe, INITBL_GetIntConfig(INITBL_OBJ, CFG_CMD_PIPE_DEPTH), INITBL_GetStrConfig(INITBL_OBJ, CFG_CMD_PIPE_NAME));  
      CFE_SB_Subscribe(AstroPiApp.CmdMid, AstroPiApp.CmdPipe);
      CFE_SB_Subscribe(AstroPiApp.SendStatusMid, AstroPiApp.CmdPipe);
      CFE_SB_Subscribe(AstroPiApp.JmsgTopicCsvTlmMid, AstroPiApp.CmdPipe);

      CMDMGR_Constructor(CMDMGR_OBJ);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, ASTRO_PI_NOOP_CC,  NULL, ASTRO_PI_APP_NoOpCmd,     0);
      CMDMGR_RegisterFunc(CMDMGR_OBJ, ASTRO_PI_RESET_CC, NULL, ASTRO_PI_APP_ResetAppCmd, 0);
      
      CMDMGR_RegisterFunc(CMDMGR_OBJ, ASTRO_PI_SEND_TEST_SCRIPT_CC,    NULL, PY_SCRIPT_SendTestCmd,    sizeof(ASTRO_PI_SendTestScript_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, ASTRO_PI_SEND_LOCAL_SCRIPT_CC,   NULL, PY_SCRIPT_SendLocalCmd,   sizeof(ASTRO_PI_SendLocalScript_CmdPayload_t));
      CMDMGR_RegisterFunc(CMDMGR_OBJ, ASTRO_PI_START_REMOTE_SCRIPT_CC, NULL, PY_SCRIPT_StartRemoteCmd, sizeof(ASTRO_PI_StartRemoteScript_CmdPayload_t));
      
      CFE_MSG_Init(CFE_MSG_PTR(AstroPiApp.StatusTlm.TelemetryHeader), CFE_SB_ValueToMsgId(INITBL_GetIntConfig(INITBL_OBJ, CFG_ASTRO_PI_STATUS_TLM_TOPICID)), sizeof(ASTRO_PI_StatusTlm_t));

      /*
      ** Application startup event message
      */
      CFE_EVS_SendEvent(ASTRO_PI_APP_INIT_APP_EID, CFE_EVS_EventType_INFORMATION,
                        "Astro Pi App Initialized. Version %d.%d.%d",
                        ASTRO_PI_APP_MAJOR_VER, ASTRO_PI_APP_MINOR_VER, ASTRO_PI_APP_PLATFORM_REV);
                        
      RetStatus = CFE_SUCCESS;
      
   } /* End if INITBL Constructed */
   
   return RetStatus;

} /* End of InitApp() */


/******************************************************************************
** Function: ProcessCommands
**
** 
*/
static int32 ProcessCommands(void)
{
   
   int32  RetStatus = CFE_ES_RunStatus_APP_RUN;
   int32  SysStatus;

   CFE_SB_Buffer_t  *SbBufPtr;
   CFE_SB_MsgId_t   MsgId = CFE_SB_INVALID_MSG_ID;


   CFE_ES_PerfLogExit(AstroPiApp.PerfId);
   SysStatus = CFE_SB_ReceiveBuffer(&SbBufPtr, AstroPiApp.CmdPipe, CFE_SB_PEND_FOREVER);
   CFE_ES_PerfLogEntry(AstroPiApp.PerfId);

   if (SysStatus == CFE_SUCCESS)
   {
      SysStatus = CFE_MSG_GetMsgId(&SbBufPtr->Msg, &MsgId);

      if (SysStatus == CFE_SUCCESS)
      {

         if (CFE_SB_MsgId_Equal(MsgId, AstroPiApp.CmdMid))
         {
            CMDMGR_DispatchFunc(CMDMGR_OBJ, &SbBufPtr->Msg);
         } 
         else if (CFE_SB_MsgId_Equal(MsgId, AstroPiApp.SendStatusMid))
         {   
            SendStatusPkt();
         }
         else if (CFE_SB_MsgId_Equal(MsgId, AstroPiApp.JmsgTopicCsvTlmMid))
         {   
            PY_SCRIPT_CreateSenseHatTlm(&SbBufPtr->Msg);
         }
         else
         {   
            CFE_EVS_SendEvent(ASTRO_PI_APP_INVALID_MID_EID, CFE_EVS_EventType_ERROR,
                              "Received invalid command packet, MID = 0x%04X(%d)", 
                              CFE_SB_MsgIdToValue(MsgId), CFE_SB_MsgIdToValue(MsgId));
         }

      } /* End if got message ID */
   } /* End if received buffer */
   else
   {
      if (SysStatus != CFE_SB_NO_MESSAGE)
      {
         RetStatus = CFE_ES_RunStatus_APP_ERROR;
      }
   } 

   return RetStatus;
   
} /* End ProcessCommands() */


/******************************************************************************
** Function: SendStatusPkt
**
*/
void SendStatusPkt(void)
{
   
   ASTRO_PI_StatusTlm_Payload_t *Payload = &AstroPiApp.StatusTlm.Payload;

   /*
   ** Framework Data
   */

   Payload->ValidCmdCnt    = AstroPiApp.CmdMgr.ValidCmdCnt;
   Payload->InvalidCmdCnt  = AstroPiApp.CmdMgr.InvalidCmdCnt;

   /*
   ** UDP Manager Data
   */

   Payload->SentScriptCnt  = AstroPiApp.PyScript.SentCnt;
   strncpy(Payload->LastSentScript, AstroPiApp.PyScript.LastSent,OS_MAX_PATH_LEN); 
       
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(AstroPiApp.StatusTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(AstroPiApp.StatusTlm.TelemetryHeader), true);

} /* End SendStatusPkt() */
