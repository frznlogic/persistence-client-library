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
 * @file           persistence_client_library_data_access.c
 * @ingroup        Persistence client library
 * @author         Ingo Huerner
 * @brief          Implementation of persistence data access
 *                 Library provides an API to access persistent data
 * @see            
 */

#include "persistence_client_library_data_access.h"

#include <string.h>
#include <stdio.h>
#include <dconf-dbus-1.h>




int get_value_from_table(GvdbTable* database, char* key, unsigned char* buffer, unsigned long buffer_size)
{
   int read_size = 0;
   gconstpointer valuePtr = NULL;
   GVariant* dbValue = NULL;

   dbValue = gvdb_table_get_value(database, key);
   
   if(dbValue != NULL)
   {
      read_size = g_variant_get_size(dbValue);
      valuePtr = g_variant_get_data(dbValue);   // get the "data" part from GVariant
      
      if( (valuePtr != NULL))
      {
         if(read_size > buffer_size)
         {
            read_size = buffer_size;   // truncate data size to buffer size
         }
         memcpy(buffer, valuePtr, read_size-1); 
      }
      else
      {
         printf("get_value_from_table:  E R R O R getting size and/or data for key: %s \n", key);
      }
   }
   else
   {
      printf("get_value_from_table:  E R R O R  getting value for key: %s \n", key);
   }

   return read_size;
} 



int get_size_from_table(GvdbTable* database, char* key)
{
   int read_size = 0;
   GVariant* dbValue = gvdb_table_get_value(database, key);

   if(dbValue != NULL)
   {
      read_size = g_variant_get_size(dbValue);
   }
   else
   {
      printf("get_size_from_table:  E R R O R getting value for key: %s \n", key);
   }

   return read_size;
}



int persistence_get_data(char* dbPath, char* key, int shared, unsigned char* buffer, unsigned long buffer_size)
{
   int read_size = -1;

   printf("key : %s \n", key);
   printf("path: %s \n", dbPath);
   if(dbShared == shared)       // check if shared data (dconf)
   {
      printf("    S H A R E D   D A T A  => not implemented yet\n");
      //DConfClient* dconf_client_new(const gchar *profile, DConfWatchFunc watch_func, gpointer user_data, GDestroyNotify notify);

      //GVariant* dconf_client_read(DConfClient *client, const gchar *key);


   }
   else if(dbLocal == shared)   // it is local data (gvdb)
   {
      GError *error = NULL;
      GvdbTable* database = gvdb_table_new(dbPath, TRUE, &error);;
      gvdb_table_ref(database);
      if(database != NULL)
      {         
         read_size = get_value_from_table(database, key, buffer, buffer_size-1);   
         gvdb_table_unref(database);
      }
      else
      {
         printf("Database  E R R O R: %s\n", error->message);
         g_error_free(error);
         error = NULL;
      }
   }
   return read_size;
}



int persistence_set_data(char* dbPath, char* key, int shared, unsigned char* buffer, unsigned long buffer_size)
{
   int write_size = -1;

   printf("key : %s \n", key);
   printf("path: %s \n", dbPath);
   if(dbShared == shared)       // check if shared data (dconf)
   {
      //GVariant *value = NULL;
      //gboolean ok = FALSE;

      printf("    S H A R E D   D A T A  => NOW IMPLEMENTING implemented yet\n");
      //DConfDBusClient *dcdbc = dconf_dbus_client_new("/com/canonical/indicator/power/", NULL, NULL);
      //ok =  dconf_dbus_client_write(dcdbc, key, value);

   }
   else if(dbLocal == shared)   // it is local data (gvdb)
   {
      GError *error = NULL;
      GHashTable* parent = NULL;      
      GHashTable* hash_table = NULL;
      const gchar *name_in_parent = "persDB";

      hash_table = gvdb_hash_table_new(parent, name_in_parent);

      if(hash_table != NULL)
      {         
         write_size = set_value_to_table(hash_table, key, buffer, buffer_size);   
         gvdb_table_write_contents(hash_table, dbPath, FALSE, &error);
      }
      else
      {
         printf("Database  E R R O R: %s\n", error->message);
         g_error_free(error);
         error = NULL;
      }
   }
   return write_size;
}



int persistence_get_data_size(char* dbPath, char* key, int shared)
{
   int read_size = -1;

   if(dbShared == shared)       // check if shared data (dconf)
   {
      printf("S H A R E D  D A T A  => not implemented yet\n");
   }
   else if(dbLocal == shared)   // it is local data (gvdb)
   {
      GError *error = NULL;
      GvdbTable* database = gvdb_table_new(dbPath, TRUE, &error);;

      if(database != 0)
      {         
         read_size = get_size_from_table(database, key);         
         gvdb_table_unref(database);
      }
      else
      {
         printf("Database  E R R O R: %s\n", error->message);
         g_error_free(error);
         error = NULL;
      }
   }
   return read_size;

}


int set_value_to_table(GHashTable* database, char* key, unsigned char* buffer, unsigned long buffer_size)
{
   int size_written = -1;
   gvdb_hash_table_insert_string(database, key,  (const gchar*)buffer);
   return size_written;
}



int persistence_reg_notify_on_change(char* dbPath, char* key)
{
   int rval = -1;

   return rval;
}







