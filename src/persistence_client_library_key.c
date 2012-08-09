/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
#include "persistence_client_library.h"
#include "persistence_client_library_handle.h"
#include "persistence_client_library_data_access.h"
#include "persistence_client_library_pas_interface.h"




// ------------------------------------------------------------------
// function with handle
// ------------------------------------------------------------------

int key_handle_open(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no)
{
   int handle = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      int shared_DB = 0;

      char dbKey[dbKeyMaxLen];      // database key
      char dbPath[dbPathMaxLen];    // database location

      // get database context: database path and database key
      shared_DB = get_db_context(ldbid, resource_id, user_no, seat_no, resIsNoFile, dbKey, dbPath);

      if(shared_DB != -1)  // check valid database context
      {
         handle = get_persistence_handle_idx();

         // remember data in handle array
         strncpy(gHandleArray[handle].dbPath, dbPath, dbPathMaxLen);
         strncpy(gHandleArray[handle].dbKey, dbKey,   dbKeyMaxLen);
         gHandleArray[handle].shared_DB = shared_DB;
      }
   }

   return handle;
}



int key_handle_close(int key_handle)
{
   int rval = 0;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      // invalidate entries
      strncpy(gHandleArray[key_handle].dbPath, "", dbPathMaxLen);
      strncpy(gHandleArray[key_handle].dbKey  ,"", dbKeyMaxLen);
      gHandleArray[key_handle].shared_DB = -1;

      set_persistence_handle_close_idx(key_handle);
   }

   return rval;
}



int key_handle_get_size(int key_handle)
{
   int size = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      if(key_handle < maxPersHandle)
         size = persistence_get_data_size(gHandleArray[key_handle].dbPath, gHandleArray[key_handle].dbKey, gHandleArray[key_handle].shared_DB);
   }

   return size;
}



int key_handle_read_data(int key_handle, unsigned char* buffer, unsigned long buffer_size)
{
   int size = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      if(key_handle < maxPersHandle)
         size = persistence_get_data(gHandleArray[key_handle].dbPath, gHandleArray[key_handle].dbKey, gHandleArray[key_handle].shared_DB, buffer, buffer_size);
   }

   return size;
}



int key_handle_register_notify_on_change(int key_handle)
{
   int rval = -1;

   return rval;
}



int key_handle_write_data(int key_handle, unsigned char* buffer, unsigned long buffer_size)
{
   int size = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      if(key_handle < maxPersHandle)
         size = persistence_set_data(gHandleArray[key_handle].dbPath, gHandleArray[key_handle].dbKey, gHandleArray[key_handle].shared_DB, buffer, buffer_size);
   }

   return size;
}




// ------------------------------------------------------------------
// functions to be used directly without a handle
// ------------------------------------------------------------------

int key_delete(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no)
{
   int rval = 0;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      // TODO
   }

   return rval;
}



// status: OK
int key_get_size(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no)
{
   int data_size = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      int shared_DB = 0;

      char dbKey[dbKeyMaxLen];      // database key
      char dbPath[dbPathMaxLen];    // database location

      // get database context: database path and database key
      shared_DB = get_db_context(ldbid, resource_id, user_no, seat_no, resIsNoFile, dbKey, dbPath);

      if(shared_DB != -1)  // check if database context is valid
      {
         data_size = persistence_get_data_size(dbPath, dbKey, shared_DB);
      }
   }

   return data_size;
}



// status: OK
int key_read_data(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no, unsigned char* buffer, unsigned long buffer_size)
{
   int data_size = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      int shared_DB = 0;

      char dbKey[dbKeyMaxLen];      // database key
      char dbPath[dbPathMaxLen];    // database location

      // get database context: database path and database key
      shared_DB = get_db_context(ldbid, resource_id, user_no, seat_no, resIsNoFile, dbKey, dbPath);

      if(shared_DB != -1)   // check if database context is valid
      {
         data_size = persistence_get_data(dbPath, dbKey, shared_DB, buffer, buffer_size);
      }
   }
   return data_size;
}



// status: TODO
int key_write_data(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no, unsigned char* buffer, unsigned long buffer_size)
{
   int data_size = -1;

   if(accessLocked == isAccessLocked() ) // check if access to persistent data is locked
   {
      int shared_DB = 0;
      unsigned int hash_val_data = 0;

      char dbKey[dbKeyMaxLen];  // database key
      char dbPath[dbPathMaxLen];    // database location

      // get database context: database path and database key
      shared_DB = get_db_context(ldbid, resource_id, user_no, seat_no, resIsNoFile, dbKey, dbPath);

      // get hash value of data to verify storing
      hash_val_data = crc32(hash_val_data, buffer, buffer_size);

      // store data
      if(shared_DB != -1)   // check if database context is valid
      {
         data_size = persistence_set_data(dbPath, dbKey, shared_DB, buffer, buffer_size);
      }
   }


   return data_size;
}



// status: TODO
int key_register_notify_on_change(unsigned char ldbid, char* resource_id, unsigned char user_no, unsigned char seat_no)
{
   int rval = 0;

//   unsigned int hash_val_data = 0;

   char dbKey[dbKeyMaxLen];  // database key 
   char dbPath[dbPathMaxLen];    // database location

   // registration is only on shared key possible
   if(dbShared == get_db_context(ldbid, resource_id, user_no, seat_no, resIsNoFile, dbKey, dbPath))
   {
      rval = persistence_reg_notify_on_change(dbPath, dbKey);
   }

   return rval;
}







