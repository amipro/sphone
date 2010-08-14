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


#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "utils.h"

#define ICONS_PATH "/usr/share/sphone/icons/"

int conf_key_set_stickiness(char *key_code, char *cmd, char *arg);
int conf_key_set_code(char *key_code, char *cmd, char *arg);
int conf_key_set_power_key(char *key_code, char *cmd, char *arg);

int debug_level=0;
void set_debug(int level){
	debug_level=level;
}
void debug(const char *s,...)
{
	if(!debug_level)
		return;
	
    va_list va;
	va_start(va,s);

    vfprintf(stdout,s,va);

    va_end(va);
}

void error(const char *s,...)
{
    va_list va;
	va_start(va,s);

    vfprintf(stderr,s,va);

    va_end(va);
}

void syserror(const char *s,...)
{
    va_list va;
	va_start(va,s);

    vfprintf(stderr,s,va);

    va_end(va);
}

void utils_audio_set(int val) {
	debug(" utils_audio_set %d\n",val);
	FILE *fout;
	char *path=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_PATH);
	if(!path)
		return;
	if(val)
		val=utils_conf_get_int(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_VALUE_ON);
	else
		val=utils_conf_get_int(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_VALUE_OFF);
	debug("utils_vibrate: write value %d to path %s\n",val,path);

	fout=fopen(path,"w");
	g_free(path);
	if(!fout) {
		error("Error opening audio file\n");
		return;
	}
	fprintf(fout,"%d",val);
	fclose(fout);
}

void utils_vibrate(int val) {
	debug(" utils_vibrate %d\n",val);
	FILE *fout;
	char *path=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_VIBRATE,UTILS_CONF_ATTR_ACTION_VIBRATE_PATH);
	if(!path)
		return;
	if(val)
		val=utils_conf_get_int(UTILS_CONF_GROUP_ACTION_VIBRATE,UTILS_CONF_ATTR_ACTION_VIBRATE_VALUE_ON);
	else
		val=utils_conf_get_int(UTILS_CONF_GROUP_ACTION_VIBRATE,UTILS_CONF_ATTR_ACTION_VIBRATE_VALUE_OFF);
	debug("utils_vibrate: write value %d to path %s\n",val,path);
	
	fout=fopen(path,"w");
	g_free(path);
	if(!fout) {
		error("Error opening vibrate file\n");
		return;
	}
	fprintf(fout,"%d",val);
	fclose(fout);
}

guint ring_timeout=0;
static int _utils_ring_stop_callback(gpointer data)
{
	utils_vibrate(0);
	return FALSE;
}
static int _utils_ring_callback(gpointer data)
{
	utils_vibrate(1);
	g_timeout_add_seconds(2,_utils_ring_stop_callback, NULL);
	return TRUE;
}

void utils_start_ringing(const gchar *dial)
{
	if(ring_timeout)
		return;
	utils_vibrate(1);
	g_timeout_add_seconds(2,_utils_ring_stop_callback, NULL);
	ring_timeout=g_timeout_add_seconds(5,_utils_ring_callback, NULL);
	utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_RINGING_ON,dial,NULL);
}

void utils_stop_ringing(const gchar *dial)
{
	if(!ring_timeout)
		return;
	g_source_remove(ring_timeout);
	ring_timeout=0;
	utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_RINGING_OFF,dial,NULL);
}

void utils_notify()
{
	utils_vibrate(1);
	g_timeout_add_seconds(2,_utils_ring_stop_callback, NULL);
}

static GdkPixbuf *photo_default=NULL;
static GdkPixbuf *photo_unknown=NULL;

GdkPixbuf *utils_get_photo_default()
{
	if(!photo_default){
		debug("utils_get_photo_default load %s\n",ICONS_PATH "contact-person.png");
		photo_default=gdk_pixbuf_new_from_file(ICONS_PATH "contact-person.png", NULL);
	}
	g_object_ref(G_OBJECT(photo_default));
	return photo_default;
}

GdkPixbuf *utils_get_photo_unknown()
{
	if(!photo_unknown){
		debug("utils_get_photo_unknown load %s\n",ICONS_PATH "contact-person-unknown.png");
		photo_unknown=gdk_pixbuf_new_from_file(ICONS_PATH "contact-person-unknown.png", NULL);
	}
	g_object_ref(G_OBJECT(photo_unknown));
	return photo_unknown;
}

GdkPixbuf *utils_get_photo(const gchar *path)
{
	return NULL;
}

GdkPixbuf *utils_get_icon(const gchar *name)
{
	gchar *path=g_build_filename(ICONS_PATH,name,NULL);
	debug("utils_get_icon load %s\n",path);
	GdkPixbuf *icon=gdk_pixbuf_new_from_file(path, NULL);
	g_free(path);
	return icon;
}

static GKeyFile *conf=NULL;
static void utils_conf_load(void)
{
	if(conf)
		return;

	conf=g_key_file_new();

	// Load system wide configuration
	g_key_file_load_from_dirs(conf,"sphone/sphone.conf",g_get_system_config_dirs(),NULL,G_KEY_FILE_NONE,NULL);

	// Override with local configuration
	gchar *localpath=g_build_filename(g_get_user_config_dir(),"sphone","sphone.conf",NULL);
	g_key_file_load_from_file(conf,localpath,G_KEY_FILE_NONE,NULL);
}

gchar *utils_conf_get_string(const gchar *group, const gchar *name)
{
	utils_conf_load();
	return g_key_file_get_string(conf,group,name,NULL);
}

gint utils_conf_get_int(const gchar *group, const gchar *name)
{
	utils_conf_load();
	return g_key_file_get_integer(conf,group,name,NULL);
}

gboolean utils_conf_has_key(const gchar *group, const gchar *name)
{
	utils_conf_load();
	return g_key_file_has_key(conf,group,name,NULL);
}

void utils_external_exec(const gchar *name, ...)
{
	debug("utils_external_exec %s\n",name);
	gchar *path=utils_conf_get_string(UTILS_CONF_GROUP_EXTERNAL,name);
	if(!path){
		debug("utils_external_exec: No " UTILS_CONF_GROUP_EXTERNAL " for %s\n",name);
		return;
	}
	
	int pid=fork();
	if(pid==-1){
		error("Fork failed\n");
		g_free(path);
		return;
	}
	
	if(pid){
		debug("Fork succeeded\n");
		g_free(path);
		return;
	}
	
	// we are now the child process
	gchar *args[10];
	int args_count=1;

	args[0]=path;
	
    va_list va;
	va_start(va,name);

	while(args_count<9){
		gchar *a=va_arg(va,gchar *);
		if(!a)
			break;
		args[args_count++]=a;
	}
	args[args_count]=NULL;
	
    va_end(va);

	setenv("SPHONE_ACTION",name,1);
	execv(path,args);
	error("utils_external_exec: execv failed %s\n",path);
	exit(0);
}
