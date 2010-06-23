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


#include <X11/Xlib.h>
#include <stdlib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "keys-grab.h"

static struct {
	GtkWidget *invisible;
}g_keys_grab;

static GdkFilterReturn g_keys_grab_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (XEvent *) xevent;
	XKeyEvent *ke = (XKeyEvent *) xev;

	if (xev->type == KeyRelease && ke->keycode==8){
		g_print("KeyRealease %d\n",ke->keycode);
	}else if (xev->type == KeyPress && ke->keycode==8){
		g_print("KeyPress %d\n",ke->keycode);
	}else
		return GDK_FILTER_CONTINUE;

	return GDK_FILTER_REMOVE;
}

int keys_grab_init(GtkWidget *window)
{
/*	gtk_widget_add_events (window,
		                 GDK_KEY_PRESS_MASK |
		                 GDK_KEY_RELEASE_MASK);
	gdk_window_add_filter (GDK_WINDOW (window->window),
		                 g_keys_grab_filter, NULL);
*/
	return 0;
}
