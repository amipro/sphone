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


#include <gtk/gtk.h>

#include "sphone-manager.h"
#include "gui-dialer.h"
#include "utils.h"
#include "gui-sms.h"
#include "gui-contact-view.h"
#include "notification.h"

#define ICONS_PATH "/usr/share/sphone/icons/"
struct{
	SphoneManager *manager;
	GtkStatusIcon *status_icon_network_strength;
	GtkStatusIcon *status_icon_network_technology;
}g_notification;

void notification_update_strength_cb();
static void notification_dial_callback(void);
static void notification_send_sms_callback(void);
static void notification_icon_activate_callback(GtkStatusIcon *status_icon, GtkWidget *menu);

int notification_init(SphoneManager *manager){
	g_notification.manager=manager;

	debug("load file %s\n",ICONS_PATH "network-strength-none.png");
	g_notification.status_icon_network_strength=gtk_status_icon_new_from_file(ICONS_PATH "network-strength-none.png");
//	g_notification.status_icon_network_technology=gtk_status_icon_new_from_file("icons/network-technology-none.png");

	notification_update_strength_cb();
	g_signal_connect(manager, "network_property_strength_change", notification_update_strength_cb, NULL);

//	gchar *technology;
//	g_object_get(manager,"technology",&technology,NULL);
//	notification_set_technology(strength);
//	g_free(technology);

	// Build the menu
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *dial_menu = gtk_menu_item_new_with_label ("Dial");
	GtkWidget *sms_menu = gtk_menu_item_new_with_label ("Send SMS");
	GtkWidget *call_hist_menu = gtk_menu_item_new_with_label ("Call History");
	GtkWidget *sms_hist_menu = gtk_menu_item_new_with_label ("SMS History");
	GtkWidget *exit_menu = gtk_menu_item_new_with_label ("Exit");
	g_signal_connect (G_OBJECT (dial_menu), "activate", G_CALLBACK (notification_dial_callback), NULL);
	g_signal_connect (G_OBJECT (sms_menu), "activate", G_CALLBACK (notification_send_sms_callback), NULL);
	g_signal_connect (G_OBJECT (call_hist_menu), "activate", G_CALLBACK (gui_history_calls), NULL);
	g_signal_connect (G_OBJECT (sms_hist_menu), "activate", G_CALLBACK (gui_history_sms), NULL);
	g_signal_connect (G_OBJECT (exit_menu), "activate", G_CALLBACK (gtk_main_quit), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), dial_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), sms_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), call_hist_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), sms_hist_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), exit_menu);
	gtk_widget_show_all (menu);
	g_signal_connect (G_OBJECT (g_notification.status_icon_network_strength), "activate", G_CALLBACK (notification_icon_activate_callback), menu);

	return 0;
}

void notification_update_strength_cb(){
	int strength=0;
	g_object_get(g_notification.manager,"strength",&strength,NULL);

	int bars=strength/20;

	debug("strength=%d bars=%d\n",strength,bars);
	if(bars>4)
		bars=4;
	if(strength>0){
		gchar *path=g_strdup_printf(ICONS_PATH "network-strength-%d.png",bars);
		debug("load file %s\n",path);
		gtk_status_icon_set_from_file(g_notification.status_icon_network_strength,path);
		g_free(path);
	}else{
		gtk_status_icon_set_from_file(g_notification.status_icon_network_strength,"icons/network-strength-none.png");
	}
}

void notification_set_technology(gchar *technology){
	if(!g_strcmp0(technology,"")){
		gtk_status_icon_set_from_file(g_notification.status_icon_network_technology,"icons/network-technology-none.png");
	}else{
		gtk_status_icon_set_from_file(g_notification.status_icon_network_technology,"icons/network-technology-none.png");
	}
}

static void notification_dial_callback(void)
{
	gui_dialer_show(NULL);
}

static void notification_send_sms_callback(void)
{
	gui_sms_send_show(NULL, NULL);
}

static void notification_icon_activate_callback(GtkStatusIcon *status_icon, GtkWidget *menu)
{
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, status_icon, 0, gtk_get_current_event_time());
}