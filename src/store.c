/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

 
 /*
 	Data model:
 		Objects:
 			dial
 				id
 				msisdn
 			contact
 				id
 				name
 				picture
 				name_coded
 			contact_dial
 				id
 				dial_id
 				contact_id
 			interaction
 				id
 				type					0:sms, 1:voice
 				direction			0: incoming, 1: outgoung
 				date
 				dial_id
 			interaction_sms
 				interaction_id
 				content				text
 			interaction_call
 				interaction_id
 				duration				in seconds
 				status					0: established, 1: missed/no answer
 */

#include <glib.h>
#include <glib-object.h>
#include <sqlite3.h>
#include <string.h>
#include <sys/stat.h>
#include "utils.h"
#include "store.h"

#define STRINGIFY(s) #s

static sqlite3 *sqlitedb=NULL;
static void store_sqlite_error(const char *module)
{
	error("%s error: %s\n",module,sqlite3_errmsg(sqlitedb));
}

/*
 	return -1: failed, 0: success, no rows, 1: success with row(s)
 */
int store_sql_exec(const gchar *sql, sqlite3_stmt **ret_stmt, GType t, ...)
{
	int ret=0;
	sqlite3_stmt *stmt=NULL;
	while(*sql){
		debug("store_sql_exec: exec %s\n",sql);
		if(stmt)
			sqlite3_finalize(stmt);
		stmt=NULL;
		if(sqlite3_prepare_v2(sqlitedb, sql, -1, &stmt, &sql)){
		   store_sqlite_error("store_sql_exec prepare statement");
			goto error;
		}
		while(*sql==';' || g_ascii_isspace(*sql))
			sql++;

		// fill-in the parameters
		va_list ap;
		va_start(ap, t);
		int i=1;
		while(t!=G_TYPE_INVALID){
			if(t==G_TYPE_STRING){
				gchar *value=va_arg(ap,gchar *);
				debug("store_sql_exec bind string %s \n",value);
				if(sqlite3_bind_text(stmt,i++,value,-1,SQLITE_STATIC)){
					store_sqlite_error("store_sql_exec sqlite3_bind_text");
					goto error_va;
				}
			}
			else if(t==G_TYPE_INT){
				int value=va_arg(ap,int);
				debug("store_sql_exec bind int %d\n",value);
				if(sqlite3_bind_int(stmt,i++,value)){
					store_sqlite_error("store_sql_exec sqlite3_bind_int");
					goto error_va;
				}
			}
			else
				error("store_sql_exec sqlite3_bind_text: Invalid parameter type\n");
			t=va_arg(ap,GType);
		}
error_va:
		va_end(ap);
		
		ret=sqlite3_step(stmt);
		if(ret==SQLITE_ROW)
			debug("store_sql_exec  row returned\n");
		if(ret!=SQLITE_DONE && ret!=SQLITE_ROW){
			store_sqlite_error("store_sql_exec step");
			goto error;
		}			
	}

	if(ret_stmt)
		*ret_stmt=stmt;
	else
		if(stmt)sqlite3_finalize(stmt);
	
	if(ret==SQLITE_ROW)
		debug("store_sql_exec  row returned2\n");
	return (ret==SQLITE_ROW?1:0);
error:
	if(stmt)sqlite3_finalize(stmt);
	return -1;
}

int store_finalize(void)
{
	return sqlite3_close(sqlitedb);
}

int store_init(void)
{
	int ret=0;
	gboolean newdb=FALSE;
	sqlite3_stmt *stmt=NULL;
	gchar *dbpath;
	debug("store_init start\n");
	dbpath=g_build_filename(g_get_user_config_dir(),"sphone",NULL);
	mkdir(dbpath,S_IREAD|S_IWRITE|S_IEXEC);
	g_free(dbpath);
	dbpath=g_build_filename(g_get_user_config_dir(),"sphone","store.sqlite",NULL);
	debug("store_init open DB %s\n",dbpath);
	ret=sqlite3_open_v2(dbpath,&sqlitedb,SQLITE_OPEN_READWRITE,NULL);
	if(ret==SQLITE_CANTOPEN){
		if(sqlitedb)sqlite3_close(sqlitedb);
		ret=sqlite3_open_v2(dbpath,&sqlitedb,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,NULL);
		newdb=TRUE;
		debug("store_init: create new store %s\n",dbpath);
	}
	g_free(dbpath);

	if(ret){
		error("store_init error: %s\n",sqlite3_errmsg(sqlitedb));
		goto error;
	}
	else
		debug("success\n");

	// Create the DB structure for the newly created DB
	if(newdb){
		if(store_sql_exec(
		             			STRINGIFY( 
									CREATE TABLE "contact" ( 
										"id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
										"name" TEXT NOT NULL,
										"source" TEXT,
										"source_ref" TEXT,
										"picture" TEXT,
										"name_coded" TEXT
									);
									CREATE TABLE "contact_dial" (
										"id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
										"dial_id" INTEGER NOT NULL,
										"contact_id" INTEGER NOT NULL
									);
									CREATE TABLE "dial" (
										"id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
										"dial" TEXT NOT NULL,
										"cleandial" TEXT NOT NULL
									);
									CREATE TABLE "interaction" (
										"id" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
										"type" INTEGER NOT NULL,
										"direction" INTEGER NOT NULL,
										"date" TEXT NOT NULL,
										"dial_id" INTEGER NOT NULL
									);
									CREATE TABLE "interaction_call" (
										"interaction_id" INTEGER NOT NULL,
										"duration" INTEGER,
										"status" INTEGER
									);
									CREATE TABLE "interaction_sms" (
										"interaction_id" INTEGER NOT NULL,
										"content" TEXT
									);
							)
		             , NULL, G_TYPE_INVALID))
			goto error;
	}

	if(stmt)sqlite3_finalize(stmt);

	return 0;
error:
	if(sqlitedb)sqlite3_close(sqlitedb);
	sqlitedb=NULL;
	if(stmt)sqlite3_finalize(stmt);
	return 1;
}

char store_dial_clean_buf[20];
// Filter all invalid characters
static char *store_dial_clean(const gchar *dial)
{
	int pos=0;
	for(;*dial;dial++)
		if(((*dial>='0' && *dial<='9') || *dial=='*' || *dial=='#') && pos<19)
			store_dial_clean_buf[pos++]=*dial;

	store_dial_clean_buf[pos]=0;

	return store_dial_clean_buf;
}

/* Get the id corresponding to a dial, or generate a new
 The matching logic is as following:
 		- Both dials will be cleared from the following characters: <whitespace>, -, (, ), +
		- exact match if length of any is less than 7
		- If shortest number of digits will be matched starting from the right
 */
int store_dial_get_id(const gchar *dial)
{
	int ret=0;
	char *cleandial=store_dial_clean(dial);

	debug("store_dial_get_id %s\n",cleandial);
	
	if(!*cleandial)
		return -1;

	sqlite3_stmt *stmt=NULL;
	
	// If less than 7 digits: exact match
	if(strlen(cleandial)<7){
		ret=store_sql_exec (STRINGIFY(
						  select id from dial where cleandial=?
						  )
			  , &stmt
			  , G_TYPE_STRING, cleandial
			  , G_TYPE_INVALID);
		if(ret<0)
			goto error;
		else if(ret>0)
			ret=sqlite3_column_int(stmt, 0);
		else
			ret=0;
		goto done;
	}

	ret=store_sql_exec (STRINGIFY(
				  select id from dial where 
	                         length(cleandial)>6 and
	                         substr(cleandial,-min(length(cleandial),length(?1)))=substr(?1,-min(length(cleandial),length(?1)))
				  )
	  , &stmt
	  , G_TYPE_STRING, cleandial
	  , G_TYPE_INVALID);
	if(ret<0)
		goto error;
	if(ret>0)
		ret=sqlite3_column_int(stmt, 0);

done:
	if(stmt)sqlite3_finalize(stmt);
	if(!ret){	// No dial was found, add one
		ret=store_sql_exec (STRINGIFY(
							  insert into dial(dial,cleandial) values(?,?)
							  )
				  , NULL
				  , G_TYPE_STRING, dial
				  , G_TYPE_STRING, cleandial
				  , G_TYPE_INVALID);
		if(!ret)
			ret=sqlite3_last_insert_rowid(sqlitedb);
	}
	return ret;

error:
	if(stmt)sqlite3_finalize(stmt);
	return -1;
}

int store_call_add(store_interaction_direction_enum direction, time_t date, const gchar *dial, store_interaction_call_status_enum status, int duration)
{
	int ret=0;
	char date_buf[25];
	struct tm  *ts;

	int dialid=store_dial_get_id(dial);
	if(dialid<0)
		return -1;
	
	ts = localtime(&date);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S %Z", ts);

	ret=store_sql_exec (STRINGIFY(
						  insert into interaction(type , direction, date, dial_id)
						  values(1, ?1, ?2, ?3)
						  )
	          , NULL
			  , G_TYPE_INT, direction
			  , G_TYPE_STRING, date_buf
			  , G_TYPE_INT, dialid
			  , G_TYPE_INVALID);
	if(!ret)
		ret=store_sql_exec (STRINGIFY(
							  insert into interaction_call(interaction_id, duration, status)
							  values(last_insert_rowid(), ?1, ?2)
							  )
				  , NULL
				  , G_TYPE_INT, duration
				  , G_TYPE_INT, status
				  , G_TYPE_INVALID);
	
	return ret;
}

int store_sms_add(store_interaction_direction_enum direction, time_t date, const gchar *dial, const gchar *content)
{
	int ret=0;
	char date_buf[25];
	struct tm  *ts;
	
	int dialid=store_dial_get_id(dial);
	if(dialid<0)
		return -1;

	ts = localtime(&date);
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M:%S %Z", ts);

	ret=store_sql_exec (STRINGIFY(
						  insert into interaction(type , direction, date, dial_id)
						  values(0, ?1, ?2, ?3)
						  )
	          , NULL
			  , G_TYPE_INT, direction
			  , G_TYPE_STRING, date_buf
			  , G_TYPE_INT, dialid
			  , G_TYPE_INVALID);
	if(!ret)
		ret=store_sql_exec (STRINGIFY(
							  insert into interaction_sms(interaction_id, content)
							  values(last_insert_rowid(), ?1)
							  )
				  , NULL
				  , G_TYPE_STRING, content
				  , G_TYPE_INVALID);
	
	return ret;
}

int store_contact_match(store_contact_struct **contact, const gchar *dial)
{
	int ret=0;
	sqlite3_stmt *stmt=NULL;
	*contact=NULL;
	int dial_id=store_dial_get_id(dial);
	if(dial_id<0)
		return -1;

	*contact=g_malloc0(sizeof(store_contact_struct));
	
	ret=store_sql_exec (
				  "select contact.id, contact.name, contact.picture from contact, contact_dial"
					"	where contact.id=contact_dial.contact_id"
					"	and contact_dial.dial_id=?"
	  , &stmt
	  , G_TYPE_INT, dial_id
	  , G_TYPE_INVALID);
	if(ret<0){
		goto done;
	}
	if(ret==0){
		store_dial_struct *dial_entry=g_malloc0(sizeof(store_dial_struct));
		dial_entry->id=dial_id;
		dial_entry->dial=g_strdup(dial);
		(*contact)->dials=g_list_append((*contact)->dials,dial_entry);
		goto done;
	}
	if(ret>0){
		int contact_id=sqlite3_column_int(stmt, 0);
		const gchar *name=(const gchar *)sqlite3_column_text(stmt, 1);
		const gchar *picture=(const gchar *)sqlite3_column_text(stmt, 2);

		(*contact)->id=contact_id;
		(*contact)->name=g_strdup(name);
		(*contact)->picture=g_strdup(picture);
	}
	
done:
	if(stmt)sqlite3_finalize(stmt);
	return ret;
}

int store_contact_load_details(store_contact_struct *contact)
{
	// No contact->id, so we can't get any more details
	debug("store_contact_load_details %d\n",contact->id);
	if(contact->id<=0)
		return 0;
	
	// Free the list if dials to refill it
	if(contact->dials){
		GList *l=contact->dials;
		store_dial_struct *entry;
		do{
			entry=(store_dial_struct *)l->data;
			g_free(entry->dial);
			g_free(entry);
			l=g_list_next(l);
		}while(l);
		g_list_free(contact->dials);
		contact->dials=NULL;
	}

	int ret=0;
	sqlite3_stmt *stmt=NULL;
	ret=store_sql_exec (
				  "select dial.id, dial.dial from dial, contact_dial where \
					contact_dial.contact_id=? and dial.id=contact_dial.dial_id"
	  , &stmt
	  , G_TYPE_INT, contact->id
	  , G_TYPE_INVALID);
	if(ret<=0){
		goto done;
	}
	if(ret>0){
		do{
			store_dial_struct *dial_entry=g_malloc0(sizeof(store_dial_struct));
			dial_entry->id=sqlite3_column_int(stmt, 0);
			dial_entry->dial=g_strdup((const gchar *)sqlite3_column_text(stmt, 1));
			contact->dials=g_list_append(contact->dials,dial_entry);

			debug("    Add dial %s\n",dial_entry->dial);
		}while(sqlite3_step(stmt)==SQLITE_ROW);
		goto done;
	}
	
done:
	if(stmt)sqlite3_finalize(stmt);
	return ret;	
}

void store_contact_free(store_contact_struct *contact)
{
	if(!contact)
		return;
	if(contact->name)
		g_free(contact->name);
	if(contact->picture)
		g_free(contact->picture);
	if(contact->dials){
		GList *l=contact->dials;
		store_dial_struct *entry;
		do{
			entry=(store_dial_struct *)l->data;
			g_free(entry->dial);
			g_free(entry);
			l=g_list_next(l);
		}while(l);
		g_list_free(contact->dials);
		contact->dials=NULL;
	}
	g_free(contact);
}

int store_contact_get_id(const gchar *name)
{
	int ret=0;

	sqlite3_stmt *stmt=NULL;
	
	ret=store_sql_exec (STRINGIFY(
					  select id from contact where name=?
					  )
		  , &stmt
		  , G_TYPE_STRING, name
		  , G_TYPE_INVALID);
	if(ret<0){
		ret=-1;
		goto error;
	}
	
	else if(ret>0)
			ret=sqlite3_column_int(stmt, 0);
	
error:
	if(stmt)sqlite3_finalize(stmt);
	return ret;
}

// codes for abcdefghijklmnopqrstuvwxyz
const gchar *nums="22233344455566677778889999";

int store_contact_add(const gchar *name)
{
	int ret=0;
	int i;

	ret=store_contact_get_id(name);
	if(ret>0)
		return ret;
	if(ret<0)
		return ret;
	
	gchar *name_coded=g_strdup(name);
	for(i=0;i<strlen(name_coded);i++){
		gchar ch=name_coded[i];
		if(!g_ascii_isalpha(ch))
			name_coded[i]='1';
		else{
			name_coded[i]=nums[g_ascii_tolower(ch)-'a'];
		}
	}
	ret=store_sql_exec (STRINGIFY(
						  insert into contact(name , name_coded)
						  values(?1, ?2)
						  )
	          , NULL
			  , G_TYPE_STRING, name
			  , G_TYPE_STRING, name_coded
			  , G_TYPE_INVALID);
	g_free(name_coded);
	
	return ret<0?ret:sqlite3_last_insert_rowid(sqlitedb);
}

int store_contact_dial_delete(gint dialid)
{
	int ret=0;
	if(dialid<1)
		return -1;

	ret=store_sql_exec (STRINGIFY(
						  delete from contact_dial where dial_id=?1
						  )
	          , NULL
			  , G_TYPE_INT, dialid
			  , G_TYPE_INVALID);
	
	return ret;
}

int store_contact_add_dial(gint contactid, const gchar *dial)
{
	int ret=0;
	int dialid=store_dial_get_id(dial);
	if(dialid<1 || contactid<1)
		return -1;

	store_contact_dial_delete(dialid);
	
	ret=store_sql_exec (STRINGIFY(
						  insert into contact_dial(contact_id, dial_id)
						  values(?1, ?2)
						  )
	          , NULL
			  , G_TYPE_INT, contactid
			  , G_TYPE_INT, dialid
			  , G_TYPE_INVALID);
	
	return ret<=0?ret:sqlite3_last_insert_rowid(sqlitedb);
}

gchar *store_interaction_sms_get_by_id(gint id)
{
	gchar *ret=NULL;

	sqlite3_stmt *stmt=NULL;
	
	if(store_sql_exec (STRINGIFY(
					  select content from interaction_sms where interaction_id=?
					  )
		  , &stmt
		  , G_TYPE_INT, id
		  , G_TYPE_INVALID)>0)
		ret=g_strdup((const gchar*)sqlite3_column_text(stmt, 0));
	
	if(stmt)sqlite3_finalize(stmt);
	return ret;
}

int store_bulk_transaction_start()
{
	return store_sql_exec (STRINGIFY(
								PRAGMA journal_mode = MEMORY;
								BEGIN TRANSACTION 
						  )
			  ,NULL, G_TYPE_INVALID);
}

int store_transaction_commit()
{
	return store_sql_exec ("COMMIT" , NULL, G_TYPE_INVALID);
}

int store_transaction_rollback()
{
	return store_sql_exec ("ROLLBACK" , NULL, G_TYPE_INVALID);
}
