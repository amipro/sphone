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
#include <gst/gst.h>
#include <alsa/asoundlib.h>
#include "utils.h"

#define ICONS_PATH "/usr/share/sphone/icons/"

int conf_key_set_stickiness(char *key_code, char *cmd, char *arg);
int conf_key_set_code(char *key_code, char *cmd, char *arg);
int conf_key_set_power_key(char *key_code, char *cmd, char *arg);
static void utils_gst_stop();
static int utils_media_play_once(gchar *path);
static int utils_media_play_repeat(gchar *path);
static int utils_alsa_route_set_play();
static int utils_alsa_route_set_incall();
static int utils_alsa_route_save();
static int utils_alsa_route_restore();

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

	if(val)
		utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_INCALL_START, NULL);
		
	char *path=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_PATH);
	if(path){
		if(val)
			val=utils_conf_get_int(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_VALUE_ON);
		else
			val=utils_conf_get_int(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_VALUE_OFF);
		debug("utils_audio_set: write value %d to path %s\n",val,path);

		fout=fopen(path,"w");
		g_free(path);
		if(!fout) {
			error("Error opening audio file\n");
		}else{
			fprintf(fout,"%d",val);
			fclose(fout);
		}
	}
	
	// Set audio routing
	if(val){
		utils_alsa_route_save();
		utils_alsa_route_set_incall();
	}else{
		utils_alsa_route_restore();
	}
	
	if(!val)
		utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_INCALL_STOP, NULL);
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

static void utils_start_ringing_vibrate()
{
	if(ring_timeout)
		return;

	utils_vibrate(1);
	g_timeout_add_seconds(2,_utils_ring_stop_callback, NULL);
	ring_timeout=g_timeout_add_seconds(5,_utils_ring_callback, NULL);
}

static int utils_ringing_state=0;
void utils_start_ringing(const gchar *dial)
{
	if(utils_ringing_state)
		return;

	if(utils_conf_get_int(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE))
		utils_start_ringing_vibrate();

	if(utils_conf_get_int(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE)){
		char *path=utils_conf_get_string(UTILS_CONF_GROUP_NOTIFICATIONS,UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH);
		
		if(path){
			if(utils_conf_get_int(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_REPEAT_ENABLE))
				utils_media_play_repeat(path);
			else
				utils_media_play_once(path);
			g_free(path);
		}
	}
	
	utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_RINGING_ON,dial,NULL);
}

void utils_stop_ringing(const gchar *dial)
{
	if(!ring_timeout)
		return;
	g_source_remove(ring_timeout);
	ring_timeout=0;
	utils_gst_stop();
	utils_external_exec(UTILS_CONF_ATTR_EXTERNAL_RINGING_OFF,dial,NULL);
}

void utils_notify()
{
	if(utils_conf_get_int(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE)){
		char *path=utils_conf_get_string(UTILS_CONF_GROUP_NOTIFICATIONS,UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH);

		if(path){
			utils_media_play_once(path);
			g_free(path);
		}
	}
	
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

/****************************************************
 	Configuration handling and parsing
 ****************************************************/

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

/****************************************************
 	External applications execution
 ****************************************************/

void utils_external_exec(const gchar *name, ...)
{
	debug("utils_external_exec %s\n",name);
	gchar *path=utils_conf_get_string(UTILS_CONF_GROUP_EXTERNAL,name);
	if(!path){
		debug("utils_external_exec: No " UTILS_CONF_GROUP_EXTERNAL " for %s\n",name);
		return;
	}
	signal(SIGCHLD, SIG_IGN);	// Prevent zombie processes

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

/****************************************************
 	Audio playback routines using gstreamer
 ****************************************************/

static GstElement *utils_gst_play;
static int utils_gst_repeat;

static int utils_gst_rewind()
{
	return gst_element_seek_simple(utils_gst_play, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0);
}

static gboolean utils_gst_bus_callback (GstBus *bus,GstMessage *message, gpointer    data)
{
	GMainLoop *loop=(GMainLoop *)data;

	switch (GST_MESSAGE_TYPE (message)) {
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
			error("Error: %s\n", err->message);
			g_error_free (err);
			g_free (debug);

			g_main_loop_quit (loop);
			break;
		}
		case GST_MESSAGE_EOS:
			/* end-of-stream */
			if(utils_gst_repeat)
				utils_gst_rewind();
			else{
				gst_element_set_state (utils_gst_play, GST_STATE_NULL);
				gst_bus_set_flushing(bus, TRUE);
				utils_gst_stop();
			}
			break;
		default:
			/* unhandled message */
			break;
	}

	return TRUE;
}

void utils_gst_init(int *argc, char ***argv)
{
	gst_init(argc, argv);
}

static int utils_gst_start(gchar *path)
{
	if(utils_gst_play)
		return 0;

	GstBus *bus;
	gchar *uri=g_filename_to_uri(path,NULL,NULL);

	if(!uri)
		return 1;

	utils_gst_play = gst_element_factory_make ("playbin2", "play");
	g_object_set (G_OBJECT (utils_gst_play), "uri", uri, NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (utils_gst_play));
	gst_bus_add_watch (bus, utils_gst_bus_callback, NULL);
	gst_object_unref (bus);

	// Set audio routing
	utils_alsa_route_save();
	utils_alsa_route_set_play();

	gst_element_set_state (utils_gst_play, GST_STATE_PLAYING);
	g_free(uri);

	return 0;
}

static void utils_gst_stop()
{
	if(!utils_gst_play)
		return ;

	gst_element_set_state (utils_gst_play, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (utils_gst_play));
	utils_gst_play=NULL;
	utils_gst_repeat=0;
	
	utils_alsa_route_restore();
}

static int utils_media_play_once(gchar *path)
{
	utils_gst_repeat=0;
	return utils_gst_start(path);
}

static int utils_media_play_repeat(gchar *path)
{
	utils_gst_repeat=1;
	return utils_gst_start(path);
}

/****************************************************
 	ALSA audio routing routines
 ****************************************************/
snd_hctl_t *utils_alsa_hctl=NULL;
snd_hctl_elem_t *utils_alsa_route_elem=NULL;
snd_ctl_elem_id_t *utils_alsa_route_elem_id=NULL;
unsigned int utils_alsa_route_play=-1;
unsigned int utils_alsa_route_incall=-1;

static int utils_alsa_init()
{
	if(utils_alsa_hctl)
		return 0;
	debug("utils_alsa_init: start\n");
	
	char *alsa_control=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_ALSA_ROUTE_CONTROL_NAME);
	char *alsa_route_play=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_ALSA_ROUTE_CONTROL_RINGING);
	char *alsa_route_incall=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_ALSA_ROUTE_CONTROL_INCALL);
	if(!alsa_control)
		return 0;

	char *alsa_device=utils_conf_get_string(UTILS_CONF_GROUP_ACTION_AUDIO,UTILS_CONF_ATTR_ACTION_AUDIO_ALSA_ROUTE_DEVICE);
	int ret=0;

	if(alsa_device)
		ret = snd_hctl_open(&utils_alsa_hctl, alsa_device, 0);
	else
		ret = snd_hctl_open(&utils_alsa_hctl, "default", 0);
	if(ret){
		error("utils_alsa_init: Alsa control device open failed: %s\n", snd_strerror(ret));
		goto done;
	}
	
	ret = snd_hctl_load(utils_alsa_hctl);
	if(ret){
		error("utils_alsa_init: Alsa control device load failed\n");
		goto done;
	}

	snd_hctl_elem_t *elem=snd_hctl_first_elem(utils_alsa_hctl);
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_info_alloca(&info);
	while(elem){
		const char *name=snd_hctl_elem_get_name(elem);
		ret=snd_hctl_elem_info(elem, info);
		if(ret){
			error("utils_alsa_init: snd_hctl_elem_info for ctrl %s error %s\n", name, snd_strerror(ret));
			continue;
		}
		
		if(!strcmp(name, alsa_control)){
			debug("utils_alsa_init: Found element %s\n", name);

			// Find the values
			int i;
			for(i=0; i<snd_ctl_elem_info_get_items(info); i++){
				snd_ctl_elem_info_set_item(info, i);
				snd_hctl_elem_info(elem, info);
				const char *s=snd_ctl_elem_info_get_item_name(info);
				debug("utils_alsa_init: check control value %s:%s\n", name, s);

				if(alsa_route_play && !strcmp(alsa_route_play, s)){
					debug("utils_alsa_init: utils_alsa_route_play=%ud\n", i);
					utils_alsa_route_play=i;
				}
				if(alsa_route_incall && !strcmp(alsa_route_incall, s)){
					debug("utils_alsa_init: utils_alsa_route_incall=%ud\n", i);
					utils_alsa_route_incall=i;
				}
			}

			snd_ctl_elem_id_malloc(&utils_alsa_route_elem_id);
			snd_ctl_elem_info_get_id(info, utils_alsa_route_elem_id);
			utils_alsa_route_elem=elem;
		}
		elem=snd_hctl_elem_next(elem);
	}
	
	debug("utils_alsa_init: ok\n");
done:
	g_free(alsa_device);
	return ret;
}

static const unsigned int utils_alsa_get_route()
{
	int err;
	snd_ctl_elem_value_t *control;
	
	if(utils_alsa_init() || !utils_alsa_route_elem || !utils_alsa_route_elem_id)
		return -1;
	debug("utils_alsa_get_route: start ...\n");

	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_value_set_id(control, utils_alsa_route_elem_id);  

	if((err=snd_hctl_elem_read(utils_alsa_route_elem, control))){
		error("utils_alsa_get_route: read failed: %s\n", snd_strerror(err));
		return -1;
	}
	
	unsigned int v=snd_ctl_elem_value_get_enumerated(control, 0);
	debug("utils_alsa_get_route: read value=%ud\n", v);

	return v;
}

static int utils_alsa_set_route(unsigned int value)
{
	if(utils_alsa_init() || !utils_alsa_route_elem || !utils_alsa_route_elem_id)
		return 1;

	debug("utils_alsa_set_control_enum: value=%ud\n", value);

	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_value_set_id(control, utils_alsa_route_elem_id);
	snd_ctl_elem_value_set_enumerated(control, 0, value);

	int err=snd_hctl_elem_write(utils_alsa_route_elem, control);
	if(err<0){
		error("utils_alsa_set_control_enum: write failed: %s\n", snd_strerror(err));
		return 1;
	}{
		debug("utils_alsa_set_control_enum: success\n");
	}

	return 0;
}


static int utils_alsa_route_set_play()
{
	if(utils_alsa_route_play==-1)
		return 0;
	
	debug("utils_alsa_route_play\n");
	int ret=utils_alsa_set_route(utils_alsa_route_play);

	if(ret)
		error("utils_alsa_route_play: failed\n");
	
	return ret;
}

static int utils_alsa_route_set_incall()
{
	if(utils_alsa_route_incall==-1)
		return 0;
	
	debug("utils_alsa_route_incall\n");
	int ret=utils_alsa_set_route(utils_alsa_route_incall);

	if(ret)
		error("utils_alsa_route_incall: failed\n");
	
	return ret;
}

static unsigned int utils_alsa_route_saved=-1;

static int utils_alsa_route_save()
{
	if(utils_alsa_route_saved!=-1){
		error("utils_alsa_route_save: try to overwrite saved value\n");
		return 1;
	}
	
	utils_alsa_route_saved=utils_alsa_get_route();
	debug("utils_alsa_route_save: saved value %ud\n", utils_alsa_route_saved);

	return 0;
}

static int utils_alsa_route_restore()
{
	if(utils_alsa_route_play==-1 && utils_alsa_route_incall==-1)
		return 0;
	
	if(utils_alsa_route_saved==-1){
		error("utils_alsa_route_restore: with no saved route\n");
		return 1;
	}

	int ret=utils_alsa_set_route(utils_alsa_route_saved);
	utils_alsa_route_saved=-1;

	return ret;
}
