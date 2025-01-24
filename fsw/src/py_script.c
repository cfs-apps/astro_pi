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
**   1. This object needs to know the JMSG_LIB script definitions
**
*/

/*
** Includes
*/

#include "py_script.h"
#include "jmsg_lib_eds_typedefs.h"
#include "jmsg_platform_eds_defines.h"

/***********************/
/** Macro Definitions **/
/***********************/


/**********************/
/** Type Definitions **/
/**********************/


/********************************** **/
/** Local File Function Prototypes **/
/************************************/

static int32 ReadScriptFile(osal_id_t FileHandle);
static void SendScriptMsg(JMSG_LIB_ExecScriptCmd_Enum_t Command, const char *CmdText, uint16 CmdTextLen);


/**********************/
/** File Global Data **/
/**********************/

static PY_SCRIPT_Class_t *PyScript;
static JMSG_LIB_ExecScriptTlm_t  ExecScriptTlm;

static char ScriptFileBuf[JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN+JMSG_PLATFORM_CHAR_BLOCK]; /* Allow one extra block to be read in case file too long*/

//static char TestScript[] = "from sense_hat import SenseHat\\nsense = SenseHat()\\nsense.show_message('Hello world')\\n";
static char TestScript[] = "print('Hello World')"; //print('Hello Astro Pi')\\n

/******************************************************************************
** Function: PY_SCRIPT_Constructor
**
** Initialize the UDP Manager object
**
** Notes:
**   1. This must be called prior to any other member functions.
**
*/
void PY_SCRIPT_Constructor(PY_SCRIPT_Class_t *PyScriptPtr, uint32 ExecScriptTlmTopicId)
{

   PyScript = PyScriptPtr;
   
   memset(PyScript, 0, sizeof(PY_SCRIPT_Class_t));

   strcpy(PyScript->LastSent, ASTRO_PI_UNDEF_TLM_STR);
   
   CFE_MSG_Init(CFE_MSG_PTR(ExecScriptTlm.TelemetryHeader), CFE_SB_ValueToMsgId(ExecScriptTlmTopicId),
                sizeof(JMSG_LIB_ExecScriptTlm_t));

} /* End PY_SCRIPT_Constructor() */


/******************************************************************************
** Function: PY_SCRIPT_ResetStatus
**
** Reset counters and status flags to a known reset state.
**
*/
void PY_SCRIPT_ResetStatus(void)
{

   PyScript->SentCnt = 0;
   strcpy(PyScript->LastSent, ASTRO_PI_UNDEF_TLM_STR);
   
} /* End PY_SCRIPT_ResetStatus() */


/******************************************************************************
** Function: PY_SCRIPT_SendLocalCmd
**
*/
bool PY_SCRIPT_SendLocalCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const ASTRO_PI_SendLocalScript_CmdPayload_t *SendLocalScriptCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, ASTRO_PI_SendLocalScript_t);   

   bool   RetStatus = false;
   int32  FileBytesRead;
   int32  SysStatus;

   osal_id_t     FileHandle;
   os_err_name_t OsErrStr;
   FileUtil_FileInfo_t FileInfo;
   
   FileInfo = FileUtil_GetFileInfo(SendLocalScriptCmd->Filename, OS_MAX_PATH_LEN, false);

   if (FILEUTIL_FILE_EXISTS(FileInfo.State))
   {

      SysStatus = OS_OpenCreate(&FileHandle, SendLocalScriptCmd->Filename, OS_FILE_FLAG_NONE, OS_READ_ONLY);

      if (SysStatus == OS_SUCCESS)
      {
         /*
         ** ReadScriptFile() - Loads ScriptFileBuf[] & returns number of bytes read 
         ** SendScriptMsg()  - Loads tlm msg payload & assumes SB success 
         */
         FileBytesRead = ReadScriptFile(FileHandle);
         if (FileBytesRead >= 0)
         { 
            OS_printf("RetStatus=%d\n",FileBytesRead);
            SendScriptMsg(JMSG_LIB_ExecScriptCmd_RUN_SCRIPT_TEXT, ScriptFileBuf, FileBytesRead);
            strncpy(PyScript->LastSent,SendLocalScriptCmd->Filename,OS_MAX_PATH_LEN);
            CFE_EVS_SendEvent(PY_SCRIPT_SEND_LOCAL_CMD_EID, CFE_EVS_EventType_INFORMATION,
                              "Sucessfully sent script %s", PyScript->LastSent);
            RetStatus = true;
         }
         else
         {
            CFE_EVS_SendEvent(PY_SCRIPT_SEND_LOCAL_CMD_EID, CFE_EVS_EventType_ERROR,
                              "Send script command failed. Error reading contents of %s", SendLocalScriptCmd->Filename);
         }
      }
      else
      {
         OS_GetErrorName(SysStatus, &OsErrStr);
         CFE_EVS_SendEvent(PY_SCRIPT_SEND_LOCAL_CMD_EID, CFE_EVS_EventType_ERROR,
                           "Send script command failed. Error opening file %s. Status = %s",
                           SendLocalScriptCmd->Filename, OsErrStr);
      }
   
   } /* End if file exists */
   else
   {
      CFE_EVS_SendEvent(PY_SCRIPT_SEND_LOCAL_CMD_EID, CFE_EVS_EventType_ERROR,
                       "Send script command failed. File %s does not exist.", SendLocalScriptCmd->Filename);
   }
         
   return RetStatus;
   
} /* End PY_SCRIPT_SendLocalCmd() */


/******************************************************************************
** Function: PY_SCRIPT_SendTestCmd
**
*/
bool PY_SCRIPT_SendTestCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   SendScriptMsg(JMSG_LIB_ExecScriptCmd_RUN_SCRIPT_TEXT, TestScript, JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN);
   strncpy(PyScript->LastSent,"Hello World test script",OS_MAX_PATH_LEN); 
   
   CFE_EVS_SendEvent(PY_SCRIPT_SEND_TEST_CMD_EID, CFE_EVS_EventType_INFORMATION,
                     "Sucessfully sent %s", PyScript->LastSent);

   return true;
      
} /* End PY_SCRIPT_SendTestCmd() */


/******************************************************************************
** Function: PY_SCRIPT_StartRemoteCmd
**
*/
bool PY_SCRIPT_StartRemoteCmd(void *DataObjPtr, const CFE_MSG_Message_t *MsgPtr)
{
   
   const ASTRO_PI_StartRemoteScript_CmdPayload_t *StartRemoteScriptCmd = CMDMGR_PAYLOAD_PTR(MsgPtr, ASTRO_PI_StartRemoteScript_t);   

   bool   RetStatus = false;
   
   
   if (FileUtil_VerifyFilenameStr(StartRemoteScriptCmd->Filename))
   {
      SendScriptMsg(JMSG_LIB_ExecScriptCmd_RUN_SCRIPT_FILE, StartRemoteScriptCmd->Filename, OS_MAX_PATH_LEN);
      strncpy(PyScript->LastSent,StartRemoteScriptCmd->Filename,OS_MAX_PATH_LEN);
      CFE_EVS_SendEvent(PY_SCRIPT_START_REMOTE_CMD_EID, CFE_EVS_EventType_INFORMATION,
                        "Sucessfully sent start remote script %s", PyScript->LastSent);
      RetStatus = true;
   }
   else
   {
      CFE_EVS_SendEvent(PY_SCRIPT_START_REMOTE_CMD_EID, CFE_EVS_EventType_ERROR,
                        "Start remote script command failed due to invalid filename");
   }
   
   return RetStatus;
   
} /* End PY_SCRIPT_StartRemoteCmd() */


/******************************************************************************
** Function: ReadScriptFile
**
** Notes:
**   1. Event messages are issued for error cases.
**
*/
static int32 ReadScriptFile(osal_id_t FileHandle)
{

   int32   RetStatus = -1;
   bool    ReadFile  = true;
   char    *ScriptFileBufPtr = ScriptFileBuf;
   int32   FileBytesRead;
   uint16  TotalBytesRead = 0;
   os_err_name_t OsErrStr;
                                
   memset(ScriptFileBuf, 0, sizeof(ScriptFileBuf));

   while (ReadFile)
   {      
      FileBytesRead = OS_read(FileHandle, ScriptFileBufPtr, JMSG_PLATFORM_CHAR_BLOCK);
      OS_printf("%d: %s\n", FileBytesRead, ScriptFileBufPtr);
      if (FileBytesRead < JMSG_PLATFORM_CHAR_BLOCK)
      {
         ReadFile = false;
         if (FileBytesRead < 0)
         {
            OS_GetErrorName(FileBytesRead, &OsErrStr);
            CFE_EVS_SendEvent(PY_SCRIPT_READ_FILE_EID, CFE_EVS_EventType_ERROR,
                              "Error reading contents of script file. Status = %s", OsErrStr);            
         }
      }
      
      TotalBytesRead   += FileBytesRead;
      ScriptFileBufPtr += FileBytesRead;
      
      if (TotalBytesRead > JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN)
      {
         ReadFile = false;
         CFE_EVS_SendEvent(PY_SCRIPT_READ_FILE_EID, CFE_EVS_EventType_ERROR,
                           "Script file length greater than %d characters", JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN);
      }

   } /* End read file loop */

   if (TotalBytesRead <= JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN)
   {
      RetStatus = TotalBytesRead;
   }
   
   OS_close(FileHandle);   

   return RetStatus;
   
} /* End ReadScriptFile() */


/******************************************************************************
** Function: SendScriptMsg
**
** Notes:
**   1. Loads script message fields and sets unused fields to defaults 
**   2. Assume valid Command passed and SB calls are successful
**
*/
static void SendScriptMsg(JMSG_LIB_ExecScriptCmd_Enum_t Command, const char *CmdText, uint16 CmdTextLen)
{

   JMSG_LIB_ExecScriptTlm_Payload_t *Payload = &ExecScriptTlm.Payload;
   
   Payload->Command = Command;

   memset(Payload->ScriptFile, 0, OS_MAX_PATH_LEN);
   memset(Payload->ScriptText, 0, JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN);
   
   if (Command == JMSG_LIB_ExecScriptCmd_RUN_SCRIPT_TEXT)
   {
      strncpy(Payload->ScriptFile, ASTRO_PI_UNDEF_TLM_STR, OS_MAX_PATH_LEN);
      strncpy(Payload->ScriptText, CmdText, CmdTextLen);
   }
   else
   {
      strncpy(Payload->ScriptFile, CmdText, CmdTextLen);
      strncpy(Payload->ScriptText, ASTRO_PI_UNDEF_TLM_STR, JMSG_PLATFORM_TOPIC_SCRIPT_STRING_MAX_LEN);      
   }

OS_printf("%s\n",Payload->ScriptFile);
OS_printf("%s\n",Payload->ScriptText);
   
   CFE_SB_TimeStampMsg(CFE_MSG_PTR(ExecScriptTlm.TelemetryHeader));
   CFE_SB_TransmitMsg(CFE_MSG_PTR(ExecScriptTlm.TelemetryHeader), true);
   
   PyScript->SentCnt++;

} /* End SendScriptMsg() */
