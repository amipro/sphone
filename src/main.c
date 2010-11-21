/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <config.h>

#include <gtk/gtk.h>
#include <unique/unique.h>

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define N_(String) (String)
#endif



#include "callbacks.h"
#include "sphone-manager.h"
#include "notification.h"
#include "gui-calls-manager.h"
#include "gui-dialer.h"
#include "utils.h"
#include "gui-sms.h"
#include "store.h"
#include "book-import.h"
#include "gui-contact-view.h"

enum {
	SPHONE_CMD_DIALER_OPEN=1,
	SPHONE_CMD_SMS_NEW,
	SPHONE_CMD_HISTORY_CALLS,
	SPHONE_CMD_HISTORY_SMS,
};

static UniqueResponse  main_message_received_callback(UniqueApp *app,gint command, UniqueMessageData *message_data, guint time_,gpointer user_data)
{
	switch(command){
		case SPHONE_CMD_DIALER_OPEN:
			gui_dialer_show(NULL);
			break;
		case SPHONE_CMD_SMS_NEW:
			gui_sms_send_show(NULL,NULL);
			break;
		case SPHONE_CMD_HISTORY_CALLS:
			gui_history_calls();
			break;
		case SPHONE_CMD_HISTORY_SMS:
			gui_history_sms();
			break;
		default:
			error("Invalid command: %d\n",command);
			return UNIQUE_RESPONSE_OK;
	}

	return UNIQUE_RESPONSE_INVALID;
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	
	gtk_set_locale ();
	gtk_init(&argc, &argv);
	utils_gst_init(&argc, &argv);

	gboolean is_cmd_dialer_open=0;
	gboolean is_cmd_sms_new=0;
	gboolean is_cmd_history_sms=0;
	gboolean is_cmd_history_calls=0;
	gboolean is_done=FALSE;
	guint c;
	
	while ((c = getopt (argc, argv, "hvc:i:")) != -1)
		switch (c){
			case '?':
			case 'h':
				error("SPhone \n%s [hvc] \n"
				      "   h\tDisplay this help\n"
				      "   v\tEnable debug\n"
				      "   c [cmd]\tExecute command. Accepted commands are: dialer-open, sms-new, history-calls, history-sms\n"
				      "   i [file]\timport contacts XML\n",argv[0]);
				return 0;
			case 'c':
				if(!g_strcmp0(optarg,"dialer-open"))
					is_cmd_dialer_open=1;
				if(!g_strcmp0(optarg,"sms-new"))
					is_cmd_sms_new=1;
				if(!g_strcmp0(optarg,"history-sms"))
					is_cmd_history_sms=1;
				if(!g_strcmp0(optarg,"history-calls"))
					is_cmd_history_calls=1;
				break;
			case 'i':
				store_init();
				book_import(optarg);
				is_done=TRUE;
				break;
			case 'v':
				set_debug(1);
				break;
       }

	UniqueApp *unique=unique_app_new_with_commands("com.xda-developers.forums.sphone",NULL
	                                               ,"dialer-open",SPHONE_CMD_DIALER_OPEN
	                                               ,"history-sms",SPHONE_CMD_HISTORY_SMS
	                                               ,"history-calls",SPHONE_CMD_HISTORY_CALLS
	                                               ,"sms-new",SPHONE_CMD_SMS_NEW,NULL);

	if(is_done);
	else if(!unique_app_is_running(unique)){
		debug("Staring new instance\n");
		SphoneManager *manager=g_object_new(sphone_manager_get_type(),NULL);
		store_init();
		notification_init(manager);
		gui_calls_manager_init(manager);
		gui_dialer_init(manager);
		gui_sms_init(manager);
		sphone_manager_populate(manager);

		if(is_cmd_dialer_open)
			gui_dialer_show(NULL);
		if(is_cmd_sms_new)
			gui_sms_send_show(NULL,NULL);
		if(is_cmd_history_sms)
			gui_history_sms();
		if(is_cmd_history_calls)
			gui_history_calls();

		g_signal_connect(G_OBJECT(unique), "message-received", G_CALLBACK(main_message_received_callback), NULL);
		
		gtk_main ();
	}else{
		debug("Instance is already running, sending commands ...\n");
		if(is_cmd_dialer_open)
			unique_app_send_message(unique,SPHONE_CMD_DIALER_OPEN,NULL);
		if(is_cmd_sms_new)
			unique_app_send_message(unique,SPHONE_CMD_SMS_NEW,NULL);
		if(is_cmd_history_sms)
			unique_app_send_message(unique,SPHONE_CMD_HISTORY_SMS,NULL);
		if(is_cmd_history_calls)
			unique_app_send_message(unique,SPHONE_CMD_HISTORY_CALLS,NULL);
	}
	
	return 0;
}
