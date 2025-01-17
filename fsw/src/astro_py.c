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
**   Manage UDP Tx and Rx interfaces
**
** Notes:
**   None
**
*/

/*
** Includes
*/

#include "astro_py.h"
#include "astro_pi_eds_defines.h"

/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/


/********************************** **/
/** Local File Function Prototypes **/
/************************************/

static bool ReadScriptFile(osal_id_t FileHandle);
static void SendScriptMsg(void);


/**********************/
/** File Global Data **/
/**********************/

static ASTRO_PY_Class_t *AstroPy;

static ASTRO_PI_ScriptTlm_t ScriptTlm;
static char ScriptFileBuf[ASTRO_PI_MAX_JMSG_TEXT_STRING_LEN+ASTRO_PI_JMSG_CHAR_BLOCK];

static char TestScript[] = "from sense_hat import SenseHat\n"
   "sense = SenseHat()\n"
   "sense.show_message(\"Hello world\")\n";
   
/******************************************************************************
** Function: ASTRO_PY_Constructor
**
** Initialize the UDP Manager object
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void ASTRO_PY_Constructor(ASTRO_PY_Class_t *AstroPyPtr, uint32 ScriptTlmTopicId)
{

   AstroPy = AstroPyPtr;
   
   memset(AstroPy, 0, sizeof(ASTRO_PY_Class_t));

   strcpy(AstroPy->LastSentScript, ASTRO_PI_UNDEF_TLM_STR);
   
   CFE_MSG_Init(CFE_MSG_PTR(ScriptTlm.TelemetryHeader), CFE_SB_ValueToMsgId(ScriptTlmTopicId),
                sizeof(ASTRO_PI_ScriptTlm_t));

} /* End ASTRO_PY_Constructor() */


/******************************************************************************
** Function: ASTRO_PY_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void ASTRO_PY_ResetStatus(void)
{

   AstroPy->SentScriptCnt = 0;
   strcpy(AstroPy->LastSentScript, ASTRO_PI_UNDEF_TLM_STR);
   
} /* End ASTRO_PY_ResetStatus() */


/******************************************************************************
** Function: ASTRO_PY_SendScriptCmd
**
*/
bool ASTRO_PY_SendScriptCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const ASTRO_PI_SendScript_CmdPayload_t *SendScriptCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, ASTRO_PI_SendScript_t);   

   bool   RetStatus = false;
   int32  SysStatus;

   osal_id_t     FileHandle;
   os_err_name_t OsErrStr;
   FileUtil_FileInfo_t FileInfo;
   
   FileInfo = FileUtil_GetFileInfo(SendScriptCmd->Filename, OS_MAX_PATH_LEN, false);

   if (FILEUTIL_FILE_EXISTS(FileInfo.State))
   {

      SysStatus = OS_OpenCreate(&FileHandle, SendScriptCmd->Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);

      if (SysStatus == OS_SUCCESS)
      {
         // ReadScriptFile() loads tlm msg buffer & SendScriptMsg() assumes SB success 
         RetStatus = ReadScriptFile(FileHandle);
         if (RetStatus)
         { 
            SendScriptMsg();
            strncpy(AstroPy->LastSentScript,SendScriptCmd->Filename,OS_MAX_PATH_LEN);
            CFE_EVS_SendEvent(ASTRO_PY_SEND_SCRIPT_CMD_EID, CFE_EVS_EventType_INFORMATION,
                              "Sucessfully sent script %s", AstroPy->LastSentScript);
         }
         else
         {
            CFE_EVS_SendEvent(ASTRO_PY_SEND_SCRIPT_CMD_EID, CFE_EVS_EventType_ERROR,
                              "Send script command failed. Error reading contents of %s", SendScriptCmd->Filename);
         }
      }
      else
      {
         OS_GetErrorName(SysStatus, &OsErrStr);
         CFE_EVS_SendEvent(ASTRO_PY_SEND_SCRIPT_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Send script command failed. Error opening file %s. Status = %s",
                           SendScriptCmd->Filename, OsErrStr);
      }
   
   } /* End if file exists */
   else
   {
      CFE_EVS_SendEvent(ASTRO_PY_SEND_SCRIPT_CMD_EID, CFE_EVS_EventType_ERROR,
                       "Send script command failed. File %s does not exist.", SendScriptCmd->Filename);
   }
         
   return RetStatus;
   
} /* End ASTRO_PY_SendScriptCmd() */


/******************************************************************************
** Function: ASTRO_PY_SendTestScriptCmd
**
*/
bool ASTRO_PY_SendTestScriptCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   strncpy(&ScriptTlm.Payload.Script[0], TestScript, ASTRO_PI_MAX_JMSG_TEXT_STRING_LEN);
   SendScriptMsg();
   strncpy(AstroPy->LastSentScript,"Hello World test script",OS_MAX_PATH_LEN); 
   CFE_EVS_SendEvent(ASTRO_PY_SEND_TEST_SCRIPT_CMD_EID, CFE_EVS_EventType_INFORMATION,
                  "Sucessfully sent %s", AstroPy->LastSentScript);

   return true;
      
} /* End ASTRO_PY_SendTestScriptCmd() */


/******************************************************************************
** Function: ReadScriptFile
**
** Notes:
**   1. Event messages are issued for error cases.
**
*/
static bool ReadScriptFile(osal_id_t FileHandle)
{

   bool    RetStatus = false;
   bool    ReadFile  = true;
   char    *ScriptFileBufPtr = ScriptFileBuf;
   int32   FileBytesRead;
   uint16  TotalBytesRead = 0;
   os_err_name_t OsErrStr;
                                
   memset(ScriptFileBuf, 0, sizeof(ScriptFileBuf));

   while (ReadFile)
   {      
      FileBytesRead = OS_read(FileHandle, ScriptFileBuf, ASTRO_PI_JMSG_CHAR_BLOCK);
      if (FileBytesRead < ASTRO_PI_JMSG_CHAR_BLOCK)
      {
         ReadFile = false;
         if (FileBytesRead < 0)
         {
            OS_GetErrorName(FileBytesRead, &OsErrStr);
            CFE_EVS_SendEvent(ASTRO_PY_READ_SCRIPT_FILE_EID, CFE_EVS_EventType_ERROR,
                              "Error reading contents of script file. Status = %s", OsErrStr);            
         }
      }
      
      TotalBytesRead   += FileBytesRead;
      ScriptFileBufPtr += FileBytesRead;
      
      if (TotalBytesRead > ASTRO_PI_MAX_JMSG_TEXT_STRING_LEN)
      {
         ReadFile = false;
         CFE_EVS_SendEvent(ASTRO_PY_READ_SCRIPT_FILE_EID, CFE_EVS_EventType_ERROR,
                           "Script file length greater than %d characters", ASTRO_PI_MAX_JMSG_TEXT_STRING_LEN);
      }

   } /* End read file loop */

   if (TotalBytesRead <= ASTRO_PI_MAX_JMSG_TEXT_STRING_LEN)
   {
      RetStatus = true;
      strncpy(&ScriptTlm.Payload.Script[0],ScriptFileBuf,TotalBytesRead);
   }
   
   OS_close(FileHandle);   

   return RetStatus;
   
} /* End ReadScriptFile() */


/******************************************************************************
** Function: SendScriptMsg
**
** Notes:
**   1. Assume SB calls successful
**
*/
static void SendScriptMsg(void)
{
OS_printf("%s\n",&ScriptTlm.Payload.Script[0]);   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(ScriptTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(ScriptTlm.TelemetryHeader), true);
   
   AstroPy->SentScriptCnt++;

} /* End SendScriptMsg() */
