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

#include "sphone-manager.h"
#include "sphone-call.h"
#include "utils.h"
#include "ofono.h"
#include "dbus-marshalers.h"
#include "store.h"

enum
{
	PROP_0,

	PROP_OPERATOR,
	PROP_STATUS,
	PROP_LOCATION_AREA_CODE,
	PROP_CELL_ID,
	PROP_TECHNOLOGY,
	PROP_STRENGTH
};

enum
{
	CALL_ADDED,
	SMS_ARRIVED,
	NETWORK_PROPERT_STRENGTH_CHANGE,
	NETWORK_PROPERT_TECHNOLOGY_CHANGE,
	NETWORK_PROPERT_OPERATOR_CHANGE,
	NETWORK_PROPERT_STATUS_CHANGE,
	LAST_SIGNAL
};

#define SPHONE_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                        SPHONE_TYPE_MANAGER, SphoneManagerPrivate))

typedef struct _SphoneManagerPrivate  SphoneManagerPrivate;

struct _SphoneManagerPrivate
{
	OfonoNetworkProperties network_properties;
	GHashTable *calls;
};

static guint manager_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (SphoneManager, sphone_manager, G_TYPE_OBJECT);

static void _sphone_manager_network_properties_callback(gpointer *data1,gchar *name, GValue *value, GObject *object)
{
	SphoneManagerPrivate *private=SPHONE_MANAGER_GET_PRIVATE(object);
	debug("_sphone_manager_network_properties_callback %s %s\n",name,G_VALUE_TYPE_NAME(value));
	if(!g_strcmp0(name,"Strength")){
		private->network_properties.strength=g_value_get_uint(value);
		g_signal_emit(object,manager_signals[NETWORK_PROPERT_STRENGTH_CHANGE],0,private->network_properties.strength);
		debug("	  value='%d'\n",private->network_properties.strength);
	}else if(!g_strcmp0(name,"CellId")){
		guint v=g_value_get_uint(value);
		//g_signal_emit(object,manager_signals[NETWORK_PROPERT_STRENGTH_CHANGE],0,v);
		debug("	  value='%d'\n",v);
	}else if(!g_strcmp0(name,"Technology")){
		g_free(private->network_properties.technology);
		private->network_properties.technology=g_value_dup_string(value);
		g_signal_emit(object,manager_signals[NETWORK_PROPERT_TECHNOLOGY_CHANGE],0,private->network_properties.technology);
		debug("	  value='%s'\n",private->network_properties.technology);
	}else if(!g_strcmp0(name,"Name")){
		g_free(private->network_properties.noperator);
		private->network_properties.noperator=g_value_dup_string(value);
		g_signal_emit(object,manager_signals[NETWORK_PROPERT_OPERATOR_CHANGE],0,private->network_properties.noperator);
		debug("	  value='%s'\n",private->network_properties.noperator);
	}else if(!g_strcmp0(name,"Status")){
		g_free(private->network_properties.status);
		private->network_properties.status=g_value_dup_string(value);
		g_signal_emit(object,manager_signals[NETWORK_PROPERT_STATUS_CHANGE],0,private->network_properties.status);
		debug("	  value='%s'\n",private->network_properties.status);
	}
}

static void sphone_manager_call_status_changed_cb(SphoneCall *call, gchar *status, SphoneManager *object){
	gchar *path;
	SphoneManagerPrivate *private=SPHONE_MANAGER_GET_PRIVATE(object);

	g_object_get(call,"dbus_path",&path,NULL);
	debug("sphone_manager_call_status_changed_cb %s\n",path);
	if(g_strcmp0("disconnected",status))
		return;
	
	g_hash_table_remove(private->calls, path);

	debug("Call table length=%d\n",g_hash_table_size(private->calls));
}

static void _sphone_manager_voice_call_manager_properties_callback(gpointer *data1,gchar *name, GValue *value, GObject *object)
{
	int i=0;
	
	SphoneManagerPrivate *private=SPHONE_MANAGER_GET_PRIVATE(object);

	if(value==NULL)
		return;
	debug("_sphone_manager_voice_call_manager_properties_callback %s %s\n",name,G_VALUE_TYPE_NAME(value));
	
	if(!g_strcmp0 (name, "Calls")){
		GPtrArray *l=g_value_get_boxed(value);
		// Add new call objects
		for(i=0;i<l->len;i++){
			char *path=g_ptr_array_index(l,i);
			debug(" check call=%s\n",g_ptr_array_index(l,i));
			if(!g_hash_table_lookup(private->calls,path)){
				debug("  Add call %s %d\n", path, g_str_hash(path));
				SphoneCall *call=g_object_new(sphone_call_get_type(),"dbus_path",path,NULL);
				g_signal_connect(call, "status_changed", G_CALLBACK(sphone_manager_call_status_changed_cb), object);
				g_hash_table_insert(private->calls,g_strdup(path),call);
				g_signal_emit(object,manager_signals[CALL_ADDED],0,call);
			}
		}
		debug("Call table length=%d\n",g_hash_table_size(private->calls));
	}
}

static void _sphone_manager_sms_incoming_callback(gpointer *data1,gchar *text, GHashTable *value, GObject *object)
{
	const gchar *from=g_value_get_string(g_hash_table_lookup(value,"Sender"));
	const gchar *_time=g_value_get_string(g_hash_table_lookup(value,"LocalSentTime"));
	
	struct tm tm;
	strptime(_time,"%Y-%m-%dT%H:%M:%S%z",&tm);
	store_sms_add(STORE_INTERACTION_DIRECTION_INCOMING, mktime(&tm),from,text);
	utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_SMS_INCOMING,from,text,NULL);
	
	debug("_sphone_manager_sms_incoming_callback %s %s %s %p\n",from,text,_time,object);
	
	g_signal_emit(object,manager_signals[SMS_ARRIVED],0,from,text,_time);
}

static void
sphone_manager_init (SphoneManager *object)
{
	SphoneManagerPrivate *private=SPHONE_MANAGER_GET_PRIVATE(object);
	debug("sphone_manager_init %p\n", object);

	ofono_init();
	ofono_read_network_properties(&private->network_properties);

	ofono_network_properties_add_handler(_sphone_manager_network_properties_callback, object);
	ofono_voice_call_manager_properties_add_handler(_sphone_manager_voice_call_manager_properties_callback, object);
	ofono_sms_incoming_add_handler(_sphone_manager_sms_incoming_callback, object);

	private->calls=g_hash_table_new_full(g_str_hash,g_str_equal, g_free, g_object_unref);
}

static void
sphone_manager_finalize (GObject *object)
{
	SphoneManagerPrivate *private=SPHONE_MANAGER_GET_PRIVATE(object);
	ofono_network_properties_free(&private->network_properties);
	ofono_network_properties_remove_handler(_sphone_manager_network_properties_callback, object);

	G_OBJECT_CLASS (sphone_manager_parent_class)->finalize (object);

	g_hash_table_destroy(private->calls);
}

static void
sphone_manager_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SPHONE_IS_MANAGER (object));

	switch (prop_id)
	{
	case PROP_OPERATOR:
		/* TODO: Add setter for "mode" property here */
		break;
	case PROP_STATUS:
		/* TODO: Add setter for "status" property here */
		break;
	case PROP_LOCATION_AREA_CODE:
		/* TODO: Add setter for "location_area_code" property here */
		break;
	case PROP_CELL_ID:
		/* TODO: Add setter for "cell_id" property here */
		break;
	case PROP_TECHNOLOGY:
		/* TODO: Add setter for "technology" property here */
		break;
	case PROP_STRENGTH:
		/* TODO: Add setter for "strength" property here */
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sphone_manager_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SPHONE_IS_MANAGER (object));

	SphoneManager *manager=SPHONE_MANAGER(object);
	SphoneManagerPrivate *private=SPHONE_MANAGER_GET_PRIVATE(manager);

	switch (prop_id)
	{
	case PROP_OPERATOR:
		g_value_set_string(value, private->network_properties.noperator);
		break;
	case PROP_STATUS:
		g_value_set_string(value, private->network_properties.status);
		break;
	case PROP_LOCATION_AREA_CODE:
		/* TODO: Add getter for "location_area_code" property here */
		break;
	case PROP_CELL_ID:
		/* TODO: Add getter for "cell_id" property here */
		break;
	case PROP_TECHNOLOGY:
		g_value_set_string(value, private->network_properties.technology);
		break;
	case PROP_STRENGTH:
		g_value_set_uint(value, private->network_properties.strength);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static 
void sphone_manager_calls_change (SphoneManager *self)
{
	/* TODO: Add default signal handler implementation here */
}

static void
sphone_manager_class_init (SphoneManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	//GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sphone_manager_finalize;
	object_class->set_property = sphone_manager_set_property;
	object_class->get_property = sphone_manager_get_property;

	klass->calls_change = sphone_manager_calls_change;

	g_object_class_install_property (object_class,
	                                 PROP_OPERATOR,
	                                 g_param_spec_string ("operator",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_STATUS,
	                                 g_param_spec_string ("status",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_LOCATION_AREA_CODE,
	                                 g_param_spec_int ("location_area_code",
	                                                   "",
	                                                   "",
	                                                   G_MININT, /* TODO: Adjust minimum property value */
	                                                   G_MAXINT, /* TODO: Adjust maximum property value */
	                                                   0,
	                                                   G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_CELL_ID,
	                                 g_param_spec_int ("cell_id",
	                                                   "",
	                                                   "",
	                                                   G_MININT, /* TODO: Adjust minimum property value */
	                                                   G_MAXINT, /* TODO: Adjust maximum property value */
	                                                   0,
	                                                   G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_TECHNOLOGY,
	                                 g_param_spec_string ("technology",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
	                                 PROP_STRENGTH,
	                                 g_param_spec_uint ("strength",
	                                                   "",
	                                                   "",
	                                                   0, 
	                                                   100, 
	                                                   0,
	                                                   G_PARAM_READABLE));

	manager_signals[CALL_ADDED] =
		g_signal_new ("call_added",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__OBJECT,
		              G_TYPE_NONE,
		              1, SPHONE_TYPE_CALL);

	manager_signals[SMS_ARRIVED] =
		g_signal_new ("sms_arrived",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			0,
			NULL, NULL,
			g_cclosure_user_marshal_VOID__STRING_STRING_STRING,
			G_TYPE_NONE,
			3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	manager_signals[NETWORK_PROPERT_STRENGTH_CHANGE] =
		g_signal_new ("network_property_strength_change",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			0,
			NULL, NULL,
			g_cclosure_marshal_VOID__UINT,
			G_TYPE_NONE,
			1, G_TYPE_UINT);

	manager_signals[NETWORK_PROPERT_STATUS_CHANGE] =
		g_signal_new ("network_property_status_change",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			0,
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1, G_TYPE_STRING);

	manager_signals[NETWORK_PROPERT_OPERATOR_CHANGE] =
		g_signal_new ("network_property_operator_change",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			0,
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1, G_TYPE_STRING);

	manager_signals[NETWORK_PROPERT_TECHNOLOGY_CHANGE] =
		g_signal_new ("network_property_technology_change",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			0,
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE,
			1, G_TYPE_STRING);

	g_type_class_add_private (klass, sizeof (SphoneManagerPrivate));
}

void sphone_manager_call_hold_and_answer(void)
{
	ofono_call_hold_and_answer();
}
void sphone_manager_call_swap(void)
{
	ofono_call_swap();
}

int sphone_manager_dial(SphoneManager *manager, const gchar *dial)
{
	return ofono_dial(dial);
}

int sphone_manager_sms_send(SphoneManager *manager, const gchar *to, const char *text)
{
	store_sms_add(STORE_INTERACTION_DIRECTION_OUTGOING, time(NULL),to,text);
	utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_SMS_OUTGOING,to,text,NULL);
	
	return ofono_sms_send(to,text);
}

void sphone_manager_populate(SphoneManager *manager)
{
	GValue *v;
	if(!ofono_voice_call_manager_get_calls(&v))
		_sphone_manager_voice_call_manager_properties_callback(NULL,"Calls",v,manager);
}
