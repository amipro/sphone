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

#ifndef _OFONO_H_
#define _OFONO_H_

typedef struct _OfonoNetworkProperties{
	gchar *status;
	gchar *technology;
	gchar *noperator;
	guint  strength;
}OfonoNetworkProperties;

typedef struct _OfonoCallProperties{
	gchar *line_identifier;
	gchar *state;
	gchar *start_time;
}OfonoCallProperties;

extern int ofono_init();
extern int ofono_read_network_properties(OfonoNetworkProperties *properties);
extern void ofono_network_properties_free(OfonoNetworkProperties *properties);
extern int ofono_network_properties_add_handler(gpointer handler, gpointer data);
extern int ofono_network_properties_remove_handler(gpointer handler, gpointer data);

extern int ofono_voice_call_manager_properties_add_handler(gpointer handler, gpointer data);
extern int ofono_voice_call_manager_properties_remove_handler(gpointer handler, gpointer data);

extern int ofono_call_properties_read(OfonoCallProperties *properties, gchar *path);
extern void ofono_call_properties_free(OfonoCallProperties *properties);
extern int ofono_voice_call_properties_add_handler(gchar *path, gpointer handler, gpointer data);
extern int ofono_voice_call_properties_remove_handler(gchar *path, gpointer handler, gpointer data);
extern int ofono_call_answer(gchar *path);
extern int ofono_call_hangup(gchar *path);
extern int ofono_call_hold_and_answer();
extern int ofono_call_swap();
extern int ofono_dial(const gchar *dial);
extern int ofono_sms_send(const gchar *to, const gchar *text);
extern int ofono_sms_incoming_add_handler(gpointer handler, gpointer data);

#endif
