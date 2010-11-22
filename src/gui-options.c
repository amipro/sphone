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

#include <config.h>
#include <gtk/gtk.h>
#include "utils.h"

struct {
	GtkWidget *main_window;
}g_options;

struct s_option{
	const gchar *group;
	const gchar *name;
};

static int gui_options_close();
static GtkWidget *gui_options_build_option_check(const gchar *group, const gchar *option, const gchar *label);
static GtkWidget *gui_options_build_option_file_audio(const gchar *group, const gchar *name, const gchar *label);
static void gui_options_set_bool_callback(GtkToggleButton *button);
static void gui_options_set_file_callback(GtkFileChooser *chooser);
static void gui_options_play_callback(GtkButton *button, GtkFileChooser *chooser);
static void gui_options_stop_callback(GtkButton *button, GtkFileChooser *chooser);

static int is_dirty=0;

void gui_options_open()
{
	if(g_options.main_window){
		gtk_window_present(GTK_WINDOW(g_options.main_window));
		return;
	}

	g_options.main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_options.main_window),"Sphone options");
	gtk_window_set_default_size(GTK_WINDOW(g_options.main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *tabs=gtk_notebook_new();
	GtkWidget *actions=gtk_hbox_new(FALSE,0);
	GtkWidget *close=gtk_button_new_from_stock(GTK_STOCK_CLOSE);

	GtkWidget *notifications_v=gtk_vbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_check(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_ENABLE, "Enable sound"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_check(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_VIBRATION_ENABLE, "Enable vibration"), FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_file_audio(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_VOICE_INCOMING_PATH, "Ring tone"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(notifications_v), gui_options_build_option_file_audio(UTILS_CONF_GROUP_NOTIFICATIONS, UTILS_CONF_ATTR_NOTIFICATIONS_SOUND_SMS_INCOMING_PATH, "SMS tone"), FALSE, FALSE, 0);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(tabs), notifications_v, gtk_label_new ("Notifications"));
	
	g_signal_connect(G_OBJECT(g_options.main_window),"delete-event", G_CALLBACK(gui_options_close),NULL);
	g_signal_connect(G_OBJECT(close),"clicked", G_CALLBACK(gui_options_close), NULL);

	gtk_box_pack_start(GTK_BOX(v1), tabs, TRUE, TRUE, 5);
	gtk_box_pack_end(GTK_BOX(actions), close, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(v1), actions, FALSE, FALSE, 5);
	gtk_container_add(GTK_CONTAINER(g_options.main_window), v1);
	
	gtk_widget_show_all(g_options.main_window);
}

static GtkWidget *gui_options_build_option_check(const gchar *group, const gchar *name, const gchar *label)
{
	GtkWidget *check=gtk_check_button_new_with_label (label);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), utils_conf_get_int(group, name));

	struct s_option *option=g_new(struct s_option, 1);
	option->group=group;
	option->name=name;
	g_object_set_data_full(G_OBJECT(check), "option", option, g_free);
	g_signal_connect(G_OBJECT(check),"toggled", G_CALLBACK(gui_options_set_bool_callback),NULL);
	
	return check;
}

static GtkWidget *gui_options_build_option_file_audio(const gchar *group, const gchar *name, const gchar *label)
{
	GtkWidget *h=gtk_hbox_new(FALSE,0);
	GtkWidget *chooser=gtk_file_chooser_button_new(label, GTK_FILE_CHOOSER_ACTION_OPEN);
	GtkWidget *play=gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	GtkWidget *stop=gtk_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
	
	gchar *default_path=utils_conf_get_string(group, name);
	if(default_path){
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(chooser), default_path);
		g_free(default_path);
	}

	GtkFileFilter *filter=gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Audio");
	gtk_file_filter_add_mime_type(filter, "audio/*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	
	struct s_option *option=g_new(struct s_option, 1);
	option->group=group;
	option->name=name;
	g_object_set_data_full(G_OBJECT(chooser), "option", option, g_free);

	gtk_box_pack_start(GTK_BOX(h), gtk_label_new(label), FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(h), chooser, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(h), play, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(h), stop, FALSE, FALSE, 5);
	
	g_signal_connect(G_OBJECT(chooser),"file-set", G_CALLBACK(gui_options_set_file_callback),NULL);
	g_signal_connect(G_OBJECT(play),"clicked", G_CALLBACK(gui_options_play_callback),chooser);
	g_signal_connect(G_OBJECT(stop),"clicked", G_CALLBACK(gui_options_stop_callback),chooser);

	return h;
}

static int gui_options_close(GtkWidget *w)
{
	if(w!=g_options.main_window)		// If the called is g_options.main_window, then it is already in the destroy process
		gtk_widget_destroy(g_options.main_window);
	g_options.main_window=NULL;

	if(is_dirty)
		utils_conf_save_local();
	is_dirty=0;
	
	return FALSE;
}

static void gui_options_set_bool_callback(GtkToggleButton *button)
{
	struct s_option *option=g_object_get_data(G_OBJECT(button), "option");
	utils_conf_set_int(option->group, option->name, gtk_toggle_button_get_active(button)==TRUE?1:0);

	is_dirty=1;
}

static void gui_options_set_file_callback(GtkFileChooser *chooser)
{
	struct s_option *option=g_object_get_data(G_OBJECT(chooser), "option");
	gchar *file=gtk_file_chooser_get_filename(chooser);
	utils_conf_set_string(option->group, option->name, file);
	g_free(file);

	is_dirty=1;
}

static void gui_options_stop_callback(GtkButton *button, GtkFileChooser *chooser)
{
	utils_media_stop();
}

static void gui_options_play_callback(GtkButton *button, GtkFileChooser *chooser)
{
	gchar *file=gtk_file_chooser_get_filename(chooser);
	utils_media_play_once(file);
	g_free(file);
}
