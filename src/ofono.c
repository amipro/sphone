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


#include <dbus/dbus-glib.h>
#include <dbus/dbus-gtype-specialized.h>
#include <dbus/dbus-protocol.h>
#include "ofono.h"
#include "dbus-marshalers.h"
#include "utils.h"

struct{
	DBusGConnection *connection;
	gchar *modem;					// The active modem path
	DBusGProxy *proxy_modem;
	DBusGProxy *proxy_network;
	DBusGProxy *proxy_manager;
	DBusGProxy *proxy_voice_call_manager;
	DBusGProxy *proxy_sms_manager;
	GHashTable *proxies_call;
}g_ofono;

DBusGProxy *ofono_proxy_modem_get()
{
	if(!g_ofono.proxy_modem){
		g_ofono.proxy_modem = dbus_g_proxy_new_for_name (g_ofono.connection,"org.ofono",g_ofono.modem,"org.ofono.Modem");
	}
	return g_ofono.proxy_modem;
}

DBusGProxy *ofono_proxy_voice_call_manager_get()
{
	if(!g_ofono.proxy_voice_call_manager){
		g_ofono.proxy_voice_call_manager = dbus_g_proxy_new_for_name (g_ofono.connection,"org.ofono",g_ofono.modem,"org.ofono.VoiceCallManager");
	}
	return g_ofono.proxy_voice_call_manager;
}

DBusGProxy *ofono_proxy_sms_manager_get()
{
	if(!g_ofono.proxy_sms_manager){
		g_ofono.proxy_sms_manager = dbus_g_proxy_new_for_name (g_ofono.connection,"org.ofono",g_ofono.modem,"org.ofono.SmsManager");
	}
	return g_ofono.proxy_sms_manager;
}

DBusGProxy *ofono_proxy_manager_get()
{
	if(!g_ofono.proxy_manager){
		g_ofono.proxy_manager = dbus_g_proxy_new_for_name (g_ofono.connection,"org.ofono","/","org.ofono.Manager");
	}
	return g_ofono.proxy_manager;
}

DBusGProxy *ofono_proxy_network_get()
{
	if(!g_ofono.proxy_network){
		g_ofono.proxy_network = dbus_g_proxy_new_for_name (g_ofono.connection,"org.ofono", g_ofono.modem,"org.ofono.NetworkRegistration");
	}
	return g_ofono.proxy_network;
}

DBusGProxy *ofono_proxy_call_get(gchar *path)
{
	DBusGProxy *proxy=g_hash_table_lookup(g_ofono.proxies_call,path);
	if(!proxy){
		proxy = dbus_g_proxy_new_for_name (g_ofono.connection,"org.ofono",path,"org.ofono.VoiceCall");
		g_hash_table_insert(g_ofono.proxies_call,g_strdup(path),proxy);
	}
	return proxy;
}

gchar *ofono_error=NULL;
gchar *ofono_get_error(void)
{
	return ofono_error;
}

// Print the dbus error message and free the error object
void error_dbus(GError *error, gchar *domain){
	if(!error)
		return;
	
	if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
		g_printerr ("%s: Caught remote method exception %s: %s\n",
		    	domain,
				dbus_g_error_get_name (error),
				error->message);
	else
		g_printerr ("%s: Error: %s\n", domain, error->message);

	if(ofono_error)
		g_free(ofono_error);
	ofono_error=g_strdup(error->message);
	
	g_error_free (error);
}

int ofono_init()
{
	GError *error;
	GHashTable *props=NULL;
	int ret=0;

	error = NULL;
	g_ofono.proxy_modem=NULL;
	g_ofono.proxy_network=NULL;
	g_ofono.proxy_manager=NULL;
	g_ofono.connection=NULL;
	g_ofono.connection = dbus_g_bus_get(DBUS_BUS_SYSTEM,&error);
	if (g_ofono.connection == NULL)	{
		error_dbus(error,"dbus_g_bus_get");
		ret=-1;
		goto error;
	}

	// get default modem
	if (!dbus_g_proxy_call (ofono_proxy_manager_get(), "GetProperties", &error, G_TYPE_INVALID,
         dbus_g_type_get_map ("GHashTable", G_TYPE_STRING,G_TYPE_VALUE), &props, G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.Manager->GetProperties");
		ret=-1;
		goto error;
    }
	GValue *v=g_hash_table_lookup(props,"Modems");
	GPtrArray *a=g_value_get_boxed(v);
	gpointer o=g_ptr_array_index(a,0);
	g_ofono.modem=g_strdup(o);

	// Activate default modem
	GValue gvalue_true={0};
	g_value_init(&gvalue_true, G_TYPE_BOOLEAN);
	g_value_set_boolean(&gvalue_true, TRUE);
	if (!dbus_g_proxy_call (ofono_proxy_modem_get(), "SetProperty", &error, G_TYPE_STRING, "Powered", G_TYPE_VALUE, &gvalue_true, G_TYPE_INVALID,
         G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.Modem->SetProperty");
		ret=-1;
		goto error;
    }
	g_value_unset(&gvalue_true);

	dbus_g_object_register_marshaller(g_cclosure_user_marshal_VOID__STRING_VALUE,G_TYPE_NONE, G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_object_register_marshaller(g_cclosure_user_marshal_VOID__STRING_VALUE,G_TYPE_NONE, G_TYPE_STRING, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), G_TYPE_INVALID);

	g_ofono.proxies_call=g_hash_table_new(g_str_hash,g_str_equal);
	
	debug("Ofono init done\n");

error:
	if(props)g_hash_table_destroy(props);
	return ret;
}

void ofono_clear()
{
	if(g_ofono.proxy_manager)g_object_unref(g_ofono.proxy_manager);
	if(g_ofono.proxy_modem)g_object_unref(g_ofono.proxy_modem);
	if(g_ofono.proxy_network)g_object_unref(g_ofono.proxy_network);
	if(g_ofono.proxy_voice_call_manager)g_object_unref(g_ofono.proxy_voice_call_manager);
}

int ofono_read_network_properties(OfonoNetworkProperties *properties)
{
	GError *error=NULL;
	GHashTable *props=NULL;
	int ret=0;

	if(properties==NULL){
		g_printerr("ofono_read_network_properties==NULL");
		goto error;
	}
	
	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	// Read network properties
	if (!dbus_g_proxy_call (ofono_proxy_network_get(), "GetProperties", &error, G_TYPE_INVALID,
         dbus_g_type_get_map ("GHashTable", G_TYPE_STRING,G_TYPE_VALUE), &props, G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.NetworkRegistration->GetProperties");
		ret=-1;
		goto error;
    }
	GValue *v=g_hash_table_lookup(props,"Status");
	properties->status=g_value_dup_string(v);
	v=g_hash_table_lookup(props,"Technology");
	properties->technology=g_value_dup_string(v);
	v=g_hash_table_lookup(props,"Operator");
	properties->noperator=g_value_dup_string(v);
	v=g_hash_table_lookup(props,"Strength");
	properties->strength=g_value_get_uint(v);

	debug("Status=%s, Technology=%s, Operator=%s, Strength=%u\n",properties->status, properties->technology, properties->noperator, (guint)properties->strength);

error:
	if(props)g_hash_table_destroy(props);
	return ret;
}

void ofono_network_properties_free(OfonoNetworkProperties *properties){
	if(!properties)
		return;

	if(properties->status)
		g_free(properties->status);
	if(properties->technology)
		g_free(properties->technology);
	if(properties->noperator)
		g_free(properties->noperator);
}

int ofono_network_properties_add_handler(gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_add_signal(ofono_proxy_network_get(),"PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(ofono_proxy_network_get(),"PropertyChanged",G_CALLBACK(handler),data,NULL);

	return ret;
}

int ofono_network_properties_remove_handler(gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_disconnect_signal(ofono_proxy_network_get(),"PropertyChanged",G_CALLBACK(handler),data);

	return ret;
}

int ofono_voice_call_manager_properties_add_handler(gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_add_signal(ofono_proxy_voice_call_manager_get(),"PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(ofono_proxy_voice_call_manager_get(),"PropertyChanged",G_CALLBACK(handler),data,NULL);

	return ret;
}

int ofono_voice_call_manager_properties_remove_handler(gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_disconnect_signal(ofono_proxy_voice_call_manager_get(),"PropertyChanged",G_CALLBACK(handler),data);

	return ret;
}

int ofono_call_properties_read(OfonoCallProperties *properties, gchar *path)
{
	GError *error=NULL;
	GHashTable *props=NULL;
	int ret=0;

	if(properties==NULL){
		g_printerr("ofono_call_properties==NULL");
		goto error;
	}
	
	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	// Read network properties
	if (!dbus_g_proxy_call (ofono_proxy_call_get(path), "GetProperties", &error, G_TYPE_INVALID,
         dbus_g_type_get_map ("GHashTable", G_TYPE_STRING,G_TYPE_VALUE), &props, G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.VoiceCall->GetProperties");
		ret=-1;
		goto error;
    }
	GValue *v=g_hash_table_lookup(props,"State");
	properties->state=g_value_dup_string(v);
	v=g_hash_table_lookup(props,"LineIdentification");
	properties->line_identifier=g_value_dup_string(v);
	v=g_hash_table_lookup(props,"StartTime");
	properties->start_time=g_value_dup_string(v);

	debug("State=%s, LineIdentifier=%s, StartTime=%s\n",properties->state, properties->line_identifier, properties->start_time);

error:
	if(props)g_hash_table_destroy(props);
	return ret;
}

void ofono_call_properties_free(OfonoCallProperties *properties){
	if(!properties)
		return;

	if(properties->state)
		g_free(properties->state);
	if(properties->line_identifier)
		g_free(properties->line_identifier);
	if(properties->start_time)
		g_free(properties->start_time);
}

int ofono_voice_call_properties_add_handler(gchar *path, gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_add_signal(ofono_proxy_call_get(path),"PropertyChanged", G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(ofono_proxy_call_get(path),"PropertyChanged",G_CALLBACK(handler),data,NULL);

	return ret;
}

int ofono_voice_call_properties_remove_handler(gchar *path, gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_disconnect_signal(ofono_proxy_call_get(path),"PropertyChanged",G_CALLBACK(handler),data);

	return ret;
}

int ofono_call_answer(gchar *path)
{
	GError *error=NULL;
	int ret=0;

	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	// Read network properties
	if (!dbus_g_proxy_call (ofono_proxy_call_get(path), "Answer", &error, G_TYPE_INVALID,
         G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.VoiceCall->Answer");
		ret=-1;
		goto error;
    }

	debug("Call answer done\n");

error:
	return ret;
}

int ofono_call_hangup(gchar *path)
{
	GError *error=NULL;
	int ret=0;

	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	// Read network properties
	if (!dbus_g_proxy_call (ofono_proxy_call_get(path), "Hangup", &error, G_TYPE_INVALID,
         G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.VoiceCall->Hangup");
		ret=-1;
		goto error;
    }

	debug("Call hangup done\n");

error:
	return ret;
}

int ofono_call_hold_and_answer()
{
	GError *error=NULL;
	int ret=0;

	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	// Read network properties
	if (!dbus_g_proxy_call (ofono_proxy_voice_call_manager_get(), "HoldAndAnswer", &error, G_TYPE_INVALID,
         G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.VoiceCall->HoldAndAnswer");
		ret=-1;
		goto error;
    }

	debug("Call HoldAndAnswer done\n");

error:
	return ret;
}

int ofono_call_swap()
{
	GError *error=NULL;
	int ret=0;

	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	if (!dbus_g_proxy_call (ofono_proxy_voice_call_manager_get(), "SwapCalls", &error, G_TYPE_INVALID,
         G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.VoiceCall->HoldAndAnswer");
		ret=-1;
		goto error;
    }

	debug("Call HoldAndAnswer done\n");

error:
	return ret;
}

int ofono_dial(const gchar *dial)
{
	GError *error=NULL;
	int ret=0;

	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	debug("ofono_dial %s\n",dial);
	if (!dbus_g_proxy_call (ofono_proxy_voice_call_manager_get(), "Dial", &error, G_TYPE_STRING, dial, G_TYPE_STRING, "", G_TYPE_INVALID,
         G_TYPE_VALUE, NULL, G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.VoiceCall->Dial");
		ret=-1;
		goto error;
    }

	debug("Call Dial done\n");

error:
	return ret;
}

int ofono_sms_send(const gchar *to, const gchar *text)
{
	GError *error=NULL;
	int ret=0;

	if (g_ofono.connection == NULL)	{
		ret=-1;
		goto error;
	}

	if (!dbus_g_proxy_call (ofono_proxy_sms_manager_get(), "SendMessage", &error, G_TYPE_STRING, to, G_TYPE_STRING, text, G_TYPE_INVALID,
         G_TYPE_INVALID))
    {
		error_dbus(error,"org.ofono.SmsManager->SendMessage");
		ret=-1;
		goto error;
    }

	debug("Call ofono_sms_send done\n");

error:
	return ret;
}

int ofono_sms_incoming_add_handler(gpointer handler, gpointer data)
{
	int ret=0;

	dbus_g_proxy_add_signal(ofono_proxy_sms_manager_get(),"IncomingMessage", G_TYPE_STRING, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(ofono_proxy_sms_manager_get(),"IncomingMessage",G_CALLBACK(handler),data,NULL);

	debug("Call ofono_sms_incoming_add_handler done\n");

	return ret;
}
