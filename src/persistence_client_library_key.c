/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_client_library_key.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence client library.
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_key.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_pas_interface.h"
#include "persistence_client_library_custom_loader.h"
#include "persistence_client_library_prct_access.h"
#include "persistence_client_library_db_access.h"
#include "crc32.h"

#include <persComRct.h>



// function declaration
int handleRegNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);
int regNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                      pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// function with handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int pclKeyHandleOpen(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int handle = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]   = {0};      // database key
      char dbPath[DbPathMaxLen] = {0};    // database location

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleOpen: "), DLT_INT(ldbid), DLT_STRING(resource_id) );
      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      handle = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
      if(   (handle >= 0)
         && (dbContext.configKey.type == PersistenceResourceType_key) )          // check if type matches
      {
         if(dbContext.configKey.storage < PersistenceStorage_LastEntry)    // check if store policy is valid
         {
            if(PersistenceStorage_custom ==  dbContext.configKey.storage)
            {
               int idx =  custom_client_name_to_id(dbPath, 1);
               char workaroundPath[128];  // workaround, because /sys/ can not be accessed on host!!!!
               snprintf(workaroundPath, 128, "%s%s", "/Data", dbPath  );

               if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_open != NULL) )
               {
                  int flag = 0, mode = 0;
                  gPersCustomFuncs[idx].custom_plugin_handle_open(workaroundPath, flag, mode);
               }
               else
               {
                  handle = EPERS_NOPLUGINFUNCT;
               }
            }

            // generate handle for custom and for normal key
            handle = get_persistence_handle_idx();

            if((handle < MaxPersHandle) && (0 <= handle))
            {
               // remember data in handle array
               strncpy(gKeyHandleArray[handle].dbPath, dbPath, DbPathMaxLen);
               strncpy(gKeyHandleArray[handle].dbKey,  dbKey,  DbKeyMaxLen);
               strncpy(gKeyHandleArray[handle].resourceID,  resource_id,  DbResIDMaxLen);
               gKeyHandleArray[handle].dbPath[DbPathMaxLen-1] = '\0'; // Ensures 0-Termination
               gKeyHandleArray[handle].dbKey[ DbKeyMaxLen-1] = '\0'; // Ensures 0-Termination
               gKeyHandleArray[handle].info = dbContext;
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyHandleOpen: error - handleId out of bounds:"), DLT_INT(handle));
            }
         }
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyHandleOpen: error - no database context or resource is not a key "));
      }
   }


   return handle;
}



int pclKeyHandleClose(int key_handle)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleClose: "),
   //                DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(key_handle < MaxPersHandle && key_handle > 0)
      {
         if(PersistenceStorage_custom == gKeyHandleArray[key_handle].info.configKey.storage )
         {
            int idx =  custom_client_name_to_id(gKeyHandleArray[key_handle].dbPath, 1);

            if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_close != NULL) )
            {
               rval = gPersCustomFuncs[idx].custom_plugin_handle_close(key_handle);
            }
            else
            {
               rval = EPERS_NOPLUGINFUNCT;
            }
         }

         if(rval != EPERS_NOPLUGINFUNCT)
         {
            set_persistence_handle_close_idx(key_handle);
            rval = 1;

            // invalidate entries
            memset(gKeyHandleArray[key_handle].dbPath, 0, DbPathMaxLen);
            memset(gKeyHandleArray[key_handle].dbKey  ,0, DbKeyMaxLen);
            gKeyHandleArray[key_handle].info.configKey.storage = -1;
         }
      }
      else
      {
         rval = EPERS_MAXHANDLE;
      }
   }

   return rval;
}



int pclKeyHandleGetSize(int key_handle)
{
   int size = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleGetSize: "),
   //                DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(key_handle < MaxPersHandle && key_handle > 0)
      {
         if(PersistenceStorage_custom ==  gKeyHandleArray[key_handle].info.configKey.storage)
         {
            int idx =  custom_client_name_to_id(gKeyHandleArray[key_handle].dbPath, 1);

            if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_get_size != NULL) )
            {
               size = gPersCustomFuncs[idx].custom_plugin_get_size(gKeyHandleArray[key_handle].dbPath);
            }
            else
            {
               size = EPERS_NOPLUGINFUNCT;
            }
         }
         else
         {
            size = persistence_get_data_size(gKeyHandleArray[key_handle].dbPath, gKeyHandleArray[key_handle].dbKey,
                                             &gKeyHandleArray[key_handle].info);
         }
      }
      else
      {
         size = EPERS_MAXHANDLE;
      }
   }

   return size;
}



int pclKeyHandleReadData(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleReadData: "),
   //             DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(key_handle < MaxPersHandle && key_handle > 0)
      {
         if(PersistenceStorage_custom ==  gKeyHandleArray[key_handle].info.configKey.storage)
         {
            int idx =  custom_client_name_to_id(gKeyHandleArray[key_handle].dbPath, 1);

            if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_get_data != NULL) )
            {
               size = gPersCustomFuncs[idx].custom_plugin_handle_get_data(key_handle, (char*)buffer, buffer_size-1);
            }
            else
            {
               size = EPERS_NOPLUGINFUNCT;
            }
         }
         else
         {
	         size = persistence_get_data(gKeyHandleArray[key_handle].dbPath, gKeyHandleArray[key_handle].dbKey,
                                        &gKeyHandleArray[key_handle].info, buffer, buffer_size);
         }
      }
      else
      {
         size = EPERS_MAXHANDLE;
      }
   }

   return size;
}



int pclKeyHandleRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback)
{
   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleRegisterNotifyOnChange: "),
   //            DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );

   return handleRegNotifyOnChange(key_handle, callback, Notify_register);
}

int pclKeyHandleUnRegisterNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback)
{
   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleUnRegisterNotifyOnChange: "),
   //         DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );

   return handleRegNotifyOnChange(key_handle, callback, Notify_unregister);
}



int handleRegNotifyOnChange(int key_handle, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      if(key_handle < MaxPersHandle && key_handle > 0)
      {
         rval = regNotifyOnChange(gKeyHandleArray[key_handle].info.context.ldbid,
                                  gKeyHandleArray[key_handle].resourceID,
                                  gKeyHandleArray[key_handle].info.context.user_no,
                                  gKeyHandleArray[key_handle].info.context.seat_no,
                                  callback,
                                  regPolicy);
      }
      else
      {
         rval = EPERS_MAXHANDLE;
      }
   }
   return rval;
}



int pclKeyHandleWriteData(int key_handle, unsigned char* buffer, int buffer_size)
{
   int size = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyHandleWriteData: "),
   //                DLT_INT(gKeyHandleArray[key_handle].info.context.ldbid), DLT_STRING(gKeyHandleArray[key_handle].resourceID) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() )     // check if access to persistent data is locked
      {
         if(buffer_size <= gMaxKeyValDataSize)  // check data size
         {
            if(key_handle < MaxPersHandle && key_handle > 0)
            {
               if(gKeyHandleArray[key_handle].info.configKey.permission != PersistencePermission_ReadOnly)  // don't write to a read only resource
               {
                  if(PersistenceStorage_custom ==  gKeyHandleArray[key_handle].info.configKey.storage)
                  {
                     int idx =  custom_client_name_to_id(gKeyHandleArray[key_handle].dbPath, 1);

                     if( (idx < PersCustomLib_LastEntry) && (gPersCustomFuncs[idx].custom_plugin_handle_set_data != NULL) )
                     {
                        size = gPersCustomFuncs[idx].custom_plugin_handle_set_data(key_handle, (char*)buffer, buffer_size-1);

                        if(size >= 0) // success ==> send change notification
                        {
                           int rval = pers_send_Notification_Signal(gKeyHandleArray[key_handle].dbKey,
                                                                  &(gKeyHandleArray[key_handle].info.context), pclNotifyStatus_changed);

                           if(rval <= 0)
                           {
                              DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyHandleWriteData: error - failed to send notification"));
                              size = rval;
                           }
                        }
                     }
                     else
                     {
                        size = EPERS_NOPLUGINFUNCT;
                     }
                  }
                  else
                  {
                     size = persistence_set_data(gKeyHandleArray[key_handle].dbPath, gKeyHandleArray[key_handle].dbKey,
                                              &gKeyHandleArray[key_handle].info, buffer, buffer_size);
                  }
               }
               else
               {
                  size = EPERS_RESOURCE_READ_ONLY;
               }

            }
            else
            {
               size = EPERS_MAXHANDLE;
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyHandleWriteData: error - buffer_size to big, limit is [bytes]:"), DLT_INT(gMaxKeyValDataSize));
            size = EPERS_MAX_BUFF_SIZE;
         }
      }
      else
      {
         size = EPERS_LOCKFS;
      }
   }

   return size;
}





// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// functions to be used directly without a handle
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int pclKeyDelete(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int rval = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyDelete: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         PersistenceInfo_s dbContext;

        char dbKey[DbKeyMaxLen]   = {0};      // database key
        char dbPath[DbPathMaxLen] = {0};    // database location

        dbContext.context.ldbid   = ldbid;
        dbContext.context.seat_no = seat_no;
        dbContext.context.user_no = user_no;

        // get database context: database path and database key
        rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
        if(   (rval >= 0)
           && (dbContext.configKey.type == PersistenceResourceType_key) )  // check if type is matching
        {
           if(   dbContext.configKey.storage < PersistenceStorage_LastEntry
              && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
           {
	           rval = persistence_delete_data(dbPath, dbKey, &dbContext);
           }
           else
           {
             rval = EPERS_BADPOL;
           }
        }
      }
      else
      {
         rval = EPERS_LOCKFS;
      }
   }

   return rval;
}



// status: OK
int pclKeyGetSize(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no)
{
   int data_size = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      PersistenceInfo_s dbContext;

      char dbKey[DbKeyMaxLen]   = {0};      // database key
      char dbPath[DbPathMaxLen] = {0};    // database location

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyGetSize: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      // get database context: database path and database key
      data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
      if(   (data_size >= 0)
         && (dbContext.configKey.type == PersistenceResourceType_key) )    // check if type matches
      {
         if(   dbContext.configKey.storage < PersistenceStorage_LastEntry
            && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
         {
            data_size = persistence_get_data_size(dbPath, dbKey, &dbContext);
         }
         else
         {
           data_size = EPERS_BADPOL;
         }
      }
      else
      {
        data_size = EPERS_BADPOL;
      }
   }

   return data_size;
}



// status: OK
int pclKeyReadData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                  unsigned char* buffer, int buffer_size)
{
   int data_size = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyReadData: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() ) // check if access to persistent data is locked
      {
         PersistenceInfo_s dbContext;

         char dbKey[DbKeyMaxLen]   = {0};      // database key
         char dbPath[DbPathMaxLen] = {0};    // database location

         dbContext.context.ldbid   = ldbid;
         dbContext.context.seat_no = seat_no;
         dbContext.context.user_no = user_no;

         // get database context: database path and database key
         data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
         if(   (data_size >= 0)
            && (dbContext.configKey.type == PersistenceResourceType_key) )
         {

            if(   dbContext.configKey.storage <  PersistenceStorage_LastEntry
               && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
            {
                  data_size = persistence_get_data(dbPath, dbKey, &dbContext, buffer, buffer_size);
            }
            else
            {
               data_size = EPERS_BADPOL;
            }
         }
         else
         {
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyReadData - error - no database context or resource is not a key"));
         }
      }
      else
      {
         data_size = EPERS_LOCKFS;
      }
   }

   return data_size;
}



int pclKeyWriteData(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no,
                   unsigned char* buffer, int buffer_size)
{
   int data_size = EPERS_NOT_INITIALIZED;

   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyWriteData: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

   if(gPclInitialized >= PCLinitialized)
   {
      if(AccessNoLock != isAccessLocked() )     // check if access to persistent data is locked
      {
         if(buffer_size <= gMaxKeyValDataSize)  // check data size
         {
            PersistenceInfo_s dbContext;

            unsigned int hash_val_data = 0;

            char dbKey[DbKeyMaxLen]   = {0};      // database key
            char dbPath[DbPathMaxLen] = {0};    // database location

            dbContext.context.ldbid   = ldbid;
            dbContext.context.seat_no = seat_no;
            dbContext.context.user_no = user_no;

            // get database context: database path and database key
            data_size = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);
            if(   (data_size >= 0)
               && (dbContext.configKey.type == PersistenceResourceType_key))
            {
               if(dbContext.configKey.permission != PersistencePermission_ReadOnly)  // don't write to a read only resource
               {
                  // get hash value of data to verify storing
                  hash_val_data = pclCrc32(hash_val_data, buffer, buffer_size);

                  // store data
                  if(   dbContext.configKey.storage <  PersistenceStorage_LastEntry
                     && dbContext.configKey.storage >= PersistenceStorage_local)   // check if store policy is valid
                  {
                     data_size = persistence_set_data(dbPath, dbKey, &dbContext, buffer, buffer_size);
                  }
                  else
                  {
                     data_size = EPERS_BADPOL;
                  }
               }
               else
               {
                  data_size = EPERS_RESOURCE_READ_ONLY;
               }
            }
            else
            {
               DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyWriteData - error - no database context or resource is not a key"));
            }
         }
         else
         {
            data_size = EPERS_BUFLIMIT;
            DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyWriteData: error - buffer_size to big, limit is [bytes]:"), DLT_INT(gMaxKeyValDataSize));
         }
      }
      else
      {
         data_size = EPERS_LOCKFS;
      }
   }
   return data_size;
}



int pclKeyUnRegisterNotifyOnChange( unsigned int  ldbid, const char *  resource_id, unsigned int  user_no, unsigned int  seat_no, pclChangeNotifyCallback_t  callback)
{
   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyUnRegisterNotifyOnChange: "),
   //            DLT_INT(ldbid), DLT_STRING(resource_id) );

   return regNotifyOnChange(ldbid, resource_id, user_no, seat_no, callback, Notify_unregister);
}


int pclKeyRegisterNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback)
{
   //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyRegisterNotifyOnChange: "),
   //            DLT_INT(ldbid), DLT_STRING(resource_id) );

   return regNotifyOnChange(ldbid, resource_id, user_no, seat_no, callback, Notify_register);
}




int regNotifyOnChange(unsigned int ldbid, const char* resource_id, unsigned int user_no, unsigned int seat_no, pclChangeNotifyCallback_t callback, PersNotifyRegPolicy_e regPolicy)
{
   int rval = EPERS_NOT_INITIALIZED;

   if(gPclInitialized >= PCLinitialized)
   {
      PersistenceInfo_s dbContext;

      //   unsigned int hash_val_data = 0;
      char dbKey[DbKeyMaxLen]   = {0};      // database key
      char dbPath[DbPathMaxLen] = {0};    // database location

      //DLT_LOG(gDLTContext, DLT_LOG_INFO, DLT_STRING("pclKeyRegisterNotifyOnChange: "), DLT_INT(ldbid), DLT_STRING(resource_id) );

      dbContext.context.ldbid   = ldbid;
      dbContext.context.seat_no = seat_no;
      dbContext.context.user_no = user_no;

      // get database context: database path and database key
      rval = get_db_context(&dbContext, resource_id, ResIsNoFile, dbKey, dbPath);

      // registration is only on shared and custom keys possible
      if(   (dbContext.configKey.storage != PersistenceStorage_local)
        && (dbContext.configKey.type    == PersistenceResourceType_key) )
      {
         rval = persistence_notify_on_change(dbKey, ldbid, user_no, seat_no, callback, regPolicy);
      }
      else
      {
         DLT_LOG(gPclDLTContext, DLT_LOG_ERROR, DLT_STRING("pclKeyRegisterNotifyOnChange: error - resource is not a shared resource or resource is not a key"));
         rval = EPERS_NOTIFY_NOT_ALLOWED;
      }
   }

   return rval;
}







