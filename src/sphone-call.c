/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#define _XOPEN_SOURCE
#include <time.h>

#include "sphone-call.h"
#include "utils.h"
#include "ofono.h"
#include "store.h"

enum
{
	PROP_0,

	PROP_STATE,
	PROP_LINE_IDENTIFIER,
	PROP_DBUS_PATH
};

enum
{
	STATUS_CHANGED,

	LAST_SIGNAL
};


static guint call_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (SphoneCall, sphone_call, G_TYPE_OBJECT);

#define SPHONE_CALL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                        SPHONE_TYPE_CALL, SphoneCallPrivate))

typedef struct _SphoneCallPrivate  SphoneCallPrivate;

struct _SphoneCallPrivate
{
	gchar *dbus_path;
	OfonoCallProperties call_properties;
	store_interaction_direction_enum direction;
	store_interaction_call_status_enum answer_status;
	guint ofono_signal_handler;
};

static void _sphone_call_properties_callback(gpointer *data1,gchar *name, GValue *value, GObject *object);

static void
sphone_call_init (SphoneCall *object)
{
	/* TODO: Add initialization code here */
}

static void
sphone_call_finalize (GObject *object)
{
	SphoneCallPrivate *private=SPHONE_CALL_GET_PRIVATE(object);
	
	debug("Clear call object \n");
	ofono_voice_call_properties_remove_handler(private->dbus_path, _sphone_call_properties_callback, object);
	if(private->dbus_path)g_free(private->dbus_path);
	ofono_call_properties_free(&private->call_properties);

	G_OBJECT_CLASS (sphone_call_parent_class)->finalize (object);
}

static void _sphone_call_properties_callback(gpointer *data1,gchar *name, GValue *value, GObject *object)
{
	SphoneCallPrivate *private=SPHONE_CALL_GET_PRIVATE(object);
	debug("_sphone_call_properties_callback %s %s\n",name,G_VALUE_TYPE_NAME(value));
	if(!g_strcmp0(name,"StartTime")){
		g_free(private->call_properties.start_time);
		private->call_properties.start_time=g_value_dup_string(value);
	}else if(!g_strcmp0(name,"State")){
		g_free(private->call_properties.state);
		private->call_properties.state=g_value_dup_string(value);
		debug("	  value='%s'\n",private->call_properties.state);

		if(!g_strcmp0(private->call_properties.state,"dialing")
		   || !g_strcmp0(private->call_properties.state,"alerting"))
			private->direction=STORE_INTERACTION_DIRECTION_OUTGOING;
		if(!g_strcmp0(private->call_properties.state,"incoming"))
			private->direction=STORE_INTERACTION_DIRECTION_INCOMING;
		if(!g_strcmp0(private->call_properties.state,"active"))
			private->answer_status=STORE_INTERACTION_CALL_STATUS_ESTABLISHED;
		if(!g_strcmp0(private->call_properties.state,"disconnected")){
			struct tm tm;
			int duration=0;
			time_t now_t=time(NULL);
			time_t start_t=now_t;
			if(private->call_properties.start_time){
				strptime(private->call_properties.start_time,"%Y-%m-%dT%H:%M:%S%z",&tm);
				start_t=mktime(&tm);
				duration=now_t-start_t;
			}
			store_call_add(private->direction, start_t,private->call_properties.line_identifier,private->answer_status, duration);
		}
		
		g_signal_emit(object,call_signals[STATUS_CHANGED],0,private->call_properties.state);
	}
}

static void
sphone_call_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SPHONE_IS_CALL (object));
	SphoneCallPrivate *private=SPHONE_CALL_GET_PRIVATE(object);

	switch (prop_id)
	{
	case PROP_DBUS_PATH:
		if(private->dbus_path)g_free(private->dbus_path);
		private->dbus_path=g_value_dup_string(value);
		ofono_call_properties_read(&private->call_properties,private->dbus_path);
		if(!g_strcmp0(private->call_properties.state,"dialing")
		   || !g_strcmp0(private->call_properties.state,"alerting"))
			private->direction=STORE_INTERACTION_DIRECTION_OUTGOING;
		if(!g_strcmp0(private->call_properties.state,"incoming"))
			private->direction=STORE_INTERACTION_DIRECTION_INCOMING;
		ofono_voice_call_properties_add_handler(private->dbus_path, _sphone_call_properties_callback, object);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sphone_call_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SPHONE_IS_CALL (object));

	SphoneCall *call=SPHONE_CALL(object);
	SphoneCallPrivate *private=SPHONE_CALL_GET_PRIVATE(call);

	switch (prop_id)
	{
	case PROP_STATE:
		g_value_set_string(value, private->call_properties.state);
		break;
	case PROP_LINE_IDENTIFIER:
		g_value_set_string(value, private->call_properties.line_identifier);
		break;
	case PROP_DBUS_PATH:
		g_value_set_string(value, private->dbus_path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sphone_call_class_init (SphoneCallClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	//GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sphone_call_finalize;
	object_class->set_property = sphone_call_set_property;
	object_class->get_property = sphone_call_get_property;

	g_object_class_install_property (object_class,
	                                 PROP_STATE,
	                                 g_param_spec_string ("state",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_LINE_IDENTIFIER,
	                                 g_param_spec_string ("line_identifier",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_DBUS_PATH,
	                                 g_param_spec_string ("dbus_path",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READWRITE));

	call_signals[STATUS_CHANGED] =
		g_signal_new ("status_changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			0,
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1, G_TYPE_STRING);

	g_type_class_add_private (klass, sizeof (SphoneCallPrivate));
}



void sphone_call_answer (SphoneCall *object)
{
	SphoneCall *call=SPHONE_CALL(object);
	SphoneCallPrivate *private=SPHONE_CALL_GET_PRIVATE(call);
	
	ofono_call_answer(private->dbus_path);
}


void sphone_call_hangup (SphoneCall *object)
{
	SphoneCall *call=SPHONE_CALL(object);
	SphoneCallPrivate *private=SPHONE_CALL_GET_PRIVATE(call);
	
	ofono_call_hangup(private->dbus_path);
}
