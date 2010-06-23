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

#ifndef _STORE_H_
#define _STORE_H_

typedef enum { STORE_INTERACTION_TYPE_SMS=0, STORE_INTERACTION_TYPE_VOICE=1} store_interaction_type_enum;
typedef enum { STORE_INTERACTION_DIRECTION_INCOMING=0, STORE_INTERACTION_DIRECTION_OUTGOING=1} store_interaction_direction_enum;
typedef enum { STORE_INTERACTION_CALL_STATUS_MISSED=0, STORE_INTERACTION_CALL_STATUS_ESTABLISHED=1} store_interaction_call_status_enum;

typedef struct{
	int id;
	gchar *dial;
}store_dial_struct;

typedef struct{
	int id;
	store_interaction_type_enum type;
	store_interaction_direction_enum direction;
	time_t date;
	gchar *dial;
	union{
		struct{
			store_interaction_call_status_enum status;
			int duration;
	    }call;
		struct{
			gchar *content;
		}sms;
	};
}store_interaction_struct;

typedef struct{
	int id;
	gchar *name;
	gchar *picture;
	GList *dials;				// store_dial_struct
}store_contact_struct;

typedef struct sqlite3_stmt sqlite3_stmt;

int store_init(void);
int store_call_add(store_interaction_direction_enum direction, time_t date, const gchar *dial, store_interaction_call_status_enum status, int duration);
int store_sms_add(store_interaction_direction_enum direction, time_t date, const gchar *dial, const gchar *content);
int store_contact_match(store_contact_struct **contact, const gchar *dial);
int store_contact_match_fuzzy(GPtrArray **contact, gchar *dial);
int store_contact_load_details(store_contact_struct *contact);
int store_contact_load_interactions(store_contact_struct *contact, store_interaction_type_enum type, store_interaction_direction_enum direction, int max_count, time_t from, time_t to);
void store_contact_free(store_contact_struct *contact);
int store_interactions_get(GPtrArray **contacts, store_interaction_type_enum type, store_interaction_direction_enum direction, int max_count, time_t from, time_t to);
int store_sql_exec(const gchar *sql, sqlite3_stmt **ret_stmt, GType t, ...);
int store_contact_add(const gchar *name);
int store_contact_add_dial(gint contactid, const gchar *dial);
int store_dial_get_id(const gchar *dial);
gchar *store_interaction_sms_get_by_id(gint id);
int store_bulk_transaction_start();
int store_transaction_commit();
int store_transaction_rollback();

#endif
