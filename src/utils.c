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
	fout=fopen("/sys/class/vogue_hw/audio","w");
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
	fout=fopen("/sys/class/htc_hw/vibrate","w");
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

void utils_start_ringing()
{
	if(ring_timeout)
		return;
	utils_vibrate(1);
	g_timeout_add_seconds(2,_utils_ring_stop_callback, NULL);
	ring_timeout=g_timeout_add_seconds(5,_utils_ring_callback, NULL);
}

void utils_stop_ringing()
{
	if(!ring_timeout)
		return;
	g_source_remove(ring_timeout);
	ring_timeout=0;
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
