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

#ifndef _UTILS_H_
#define _UTILS_H_
 
#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#define TEST_BIT(x,addr) (1UL & (addr[x/8] >> (x & 0xff)))

void set_debug(int level);
void debug(const char *s,...); 
void syserror(const char *s,...);
void error(const char *s,...);

void utils_vibrate(int val);
void utils_audio_set(int val);
void utils_start_ringing();
void utils_stop_ringing();
void utils_notify();

#include <gtk/gtk.h>
GdkPixbuf *utils_get_photo_default();
GdkPixbuf *utils_get_photo_unknown();
GdkPixbuf *utils_get_photo(const gchar *path);
GdkPixbuf *utils_get_icon(const gchar *name);

#endif
