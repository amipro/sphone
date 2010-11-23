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
#include "sphone-manager.h"
#include "sphone-call.h"
#include "utils.h"
#include "gui-calls-manager.h"
#include "gui-contact-view.h"
#include "store.h"
#include "notification.h"

struct {
	SphoneManager *manager;
	GtkWidget *main_window;
	GtkListStore *dials_store;
	GtkWidget *dials_view;
	GtkWidget *answer_waiting_button;
	GtkWidget *answer_button;
	GtkWidget *hangup_button;
	GtkWidget *activate_button;
	GtkWidget *mute_button;
	GtkWidget *speaker_button;
	GtkWidget *handset_button;
}g_calls_manager;

enum{
	GUI_CALLS_COLUMN_STATUS,
	GUI_CALLS_COLUMN_DIAL,
	GUI_CALLS_COLUMN_CALL,
	GUI_CALLS_COLUMN_DESC,
	GUI_CALLS_COLUMN_PHOTO
};

static void gui_calls_select_callback();
static void gui_calls_double_click_callback();
static void gui_calls_utils_add_dial(gchar *dial, gchar *status, SphoneCall *call);
static void gui_calls_new_call_callback(SphoneManager *manager, SphoneCall *call);
static void gui_calls_utils_update_dial(gchar *dial, gchar *status);
static void gui_calls_utils_delete_dial(gchar *dial);
static void gui_calls_answer_callback();
static void gui_calls_hangup_callback();
static void gui_calls_answer_waiting_callback();
static void gui_calls_activate_callback();
static void gui_calls_mute_callback();
static void gui_calls_speaker_callback();
static void gui_calls_handset_callback();

static gboolean return_true(void){return TRUE;}

int gui_calls_manager_init(SphoneManager *manager)
{
	g_calls_manager.manager=manager;
	g_calls_manager.main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_calls_manager.main_window),"Active Calls");
	gtk_window_set_deletable(GTK_WINDOW(g_calls_manager.main_window),FALSE);
	gtk_window_set_default_size(GTK_WINDOW(g_calls_manager.main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	g_calls_manager.dials_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_calls_manager.dials_view),FALSE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Photo", renderer, "pixbuf", GUI_CALLS_COLUMN_PHOTO, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_calls_manager.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", GUI_CALLS_COLUMN_DESC, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_calls_manager.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Status", renderer, "text", GUI_CALLS_COLUMN_STATUS, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_calls_manager.dials_view), column);

	g_calls_manager.dials_store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, SPHONE_TYPE_CALL, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	gtk_tree_view_set_model(GTK_TREE_VIEW(g_calls_manager.dials_view), GTK_TREE_MODEL(g_calls_manager.dials_store));

	GtkWidget *h1=gtk_hbox_new(FALSE, 0);
	GtkWidget *h2=gtk_hbox_new(FALSE, 0);
	
	gtk_container_add(GTK_CONTAINER(s), g_calls_manager.dials_view);
	gtk_container_add(GTK_CONTAINER(v1), s);
	gtk_box_pack_start(GTK_BOX(v1), h1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(v1), h2, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(g_calls_manager.main_window), v1);

	gtk_widget_show_all(v1);

	g_calls_manager.answer_button=gtk_button_new_with_label("Answer");
	g_calls_manager.answer_waiting_button=gtk_button_new_with_label("Answer");
	g_calls_manager.activate_button=gtk_button_new_with_label("Activate");
	g_calls_manager.hangup_button=gtk_button_new_with_label("Hangup");
	g_calls_manager.mute_button=gtk_button_new_with_label("Mute ringing");
	g_calls_manager.speaker_button=gtk_button_new_with_label("Speaker");
	g_calls_manager.handset_button=gtk_button_new_with_label("Handset");
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.activate_button);
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.answer_button);
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.answer_waiting_button);
	gtk_container_add(GTK_CONTAINER(h1), g_calls_manager.hangup_button);
	gtk_container_add(GTK_CONTAINER(h2), g_calls_manager.mute_button);
	gtk_container_add(GTK_CONTAINER(h2), g_calls_manager.speaker_button);
	gtk_container_add(GTK_CONTAINER(h2), g_calls_manager.handset_button);

	g_signal_connect(G_OBJECT(g_calls_manager.dials_view),"cursor-changed", G_CALLBACK(gui_calls_select_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.dials_view),"row-activated", G_CALLBACK(gui_calls_double_click_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.activate_button),"clicked", G_CALLBACK(gui_calls_activate_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.answer_button),"clicked", G_CALLBACK(gui_calls_answer_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.answer_waiting_button),"clicked", G_CALLBACK(gui_calls_answer_waiting_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.hangup_button),"clicked", G_CALLBACK(gui_calls_hangup_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.mute_button),"clicked", G_CALLBACK(gui_calls_mute_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.speaker_button),"clicked", G_CALLBACK(gui_calls_speaker_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.handset_button),"clicked", G_CALLBACK(gui_calls_handset_callback),NULL);
	g_signal_connect(G_OBJECT(g_calls_manager.main_window),"delete-event", G_CALLBACK(return_true),NULL);

	g_signal_connect(G_OBJECT(manager),"call_added", G_CALLBACK(gui_calls_new_call_callback),NULL);

	return 0;
}

/*
 check if the voice channel should be enabled
 */
static int gui_calls_voice_status=0;
static void gui_calls_check_voice(void)
{
	int enable_voice=FALSE;
	GValue value={0};
	GtkTreeIter iter;	
	
	int r=gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter);
	while(r){
		gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,0,&value);
		const gchar *state=g_value_get_string(&value);
		if(!g_strcmp0(state,"active")
		   || !g_strcmp0(state,"dialing")
		   || !g_strcmp0(state,"alerting")){
			enable_voice=TRUE;
		}
		g_value_unset(&value);
		r=gtk_tree_model_iter_next(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter);
	}

	if(enable_voice)
		gui_calls_voice_status=1;
	else
		gui_calls_voice_status=0;
	
	utils_audio_set(gui_calls_voice_status);
}

static void gui_calls_update_global_status()
{
	if(utils_ringing_status())
		gtk_widget_show(g_calls_manager.mute_button);
	else
		gtk_widget_hide(g_calls_manager.mute_button);

	if(gui_calls_voice_status){
		int route=utils_audio_route_get();
		if(route==UTILS_AUDIO_ROUTE_SPEAKER || !utils_audio_route_check(UTILS_AUDIO_ROUTE_SPEAKER))
			gtk_widget_hide(g_calls_manager.speaker_button);
		else
			gtk_widget_show(g_calls_manager.speaker_button);

		if(route==UTILS_AUDIO_ROUTE_HANDSET || !utils_audio_route_check(UTILS_AUDIO_ROUTE_HANDSET))
			gtk_widget_hide(g_calls_manager.handset_button);
		else
			gtk_widget_show(g_calls_manager.handset_button);
	}
}

static void gui_calls_call_status_callback(SphoneCall *call)
{
	gchar *dial=NULL;
	gchar *state=NULL;
	gint answer_status, direction;
	
	debug("gui_calls_call_status_callback\n");
	
	g_object_get( G_OBJECT(call), "line_identifier", &dial, "state", &state, "answer_status", &answer_status, "direction", &direction, NULL);
	debug("Update call %s %s\n",dial,state);
	if(!g_strcmp0 (state,"incoming"))
		utils_start_ringing(dial);
	else
		utils_stop_ringing(dial);

	if(!g_strcmp0 (state,"disconnected")){
		gui_calls_utils_delete_dial(dial);
		if(answer_status==STORE_INTERACTION_CALL_STATUS_MISSED && direction==STORE_INTERACTION_DIRECTION_INCOMING){
			notification_add("missed_call.png",gui_history_calls);
		}
	}else
		gui_calls_utils_update_dial(dial,state);

	gui_calls_check_voice();

	gui_calls_update_global_status();
	
	g_free(state);
	g_free(dial);
}

static void gui_calls_new_call_callback(SphoneManager *manager, SphoneCall *call)
{
	gchar *dial=NULL;
	gchar *state=NULL;
	
	debug("gui_calls_new_call_callback\n");
	
	g_object_get( G_OBJECT(call), "line_identifier", &dial, "state", &state, NULL);
	debug("Add new call %s %s\n",dial,state);
	gui_calls_utils_add_dial(dial,state,call);
	g_signal_connect(G_OBJECT(call), "status_changed", G_CALLBACK(gui_calls_call_status_callback), NULL);
	gui_calls_check_voice();

	if(!g_strcmp0 (state,"incoming"))
		utils_start_ringing(dial);
	else
		utils_stop_ringing(dial);

	gui_calls_update_global_status();
	
	g_free(state);
	g_free(dial);
}

static void gui_calls_select_callback()
{
	debug("gui_calls_select_callback\n");
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value={0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,0,&value);
	const gchar *status=g_value_get_string(&value);

	if(!g_strcmp0(status,"active")){
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}else if(!g_strcmp0(status,"held")){
		gtk_widget_show(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}else if(!g_strcmp0(status,"dialing")){
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}else if(!g_strcmp0(status,"alerting")){
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}else if(!g_strcmp0(status,"incoming")){
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_show(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}else if(!g_strcmp0(status,"waiting")){
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_show(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}else{
		gtk_widget_hide(g_calls_manager.activate_button);
		gtk_widget_hide(g_calls_manager.answer_waiting_button);
		gtk_widget_hide(g_calls_manager.answer_button);
		gtk_widget_show(g_calls_manager.hangup_button);
	}
	
	g_value_unset(&value);
	                         
}

static void gui_calls_double_click_callback()
{
	debug("gui_calls_select_callback\n");
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value={0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,GUI_CALLS_COLUMN_DIAL,&value);
	const gchar *dial=g_value_get_string(&value);

	gui_contact_open_by_dial(dial);
	
	g_value_unset(&value);
	                         
}

static void gui_calls_utils_delete_dial(gchar *dial)
{
	GtkTreeIter iter;
	GValue value={0};

	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter)){
		return;
	}
	do{
		gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,1,&value);
		const gchar *idial=g_value_get_string(&value);
		if(!g_strcmp0(dial,idial)){
			gtk_list_store_remove(g_calls_manager.dials_store, &iter);
		}
		g_value_unset(&value);
	}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter));

	// Hide the window if no active calls
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter)){
		gtk_widget_hide(g_calls_manager.main_window);
	}else{
		GtkTreePath *path=gtk_tree_model_get_path(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),path,NULL,FALSE);
		gtk_tree_path_free(path);
	}
}

static void gui_calls_utils_update_dial(gchar *dial, gchar *status)
{
	GtkTreeIter iter;
	GValue value={0};

	debug("try update dial %s %s \n",dial,status);
	if(!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter))
	   return;
	do{
		gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,1,&value);
		const gchar *idial=g_value_get_string(&value);
		if(!g_strcmp0(dial,idial)){
			gtk_list_store_set(g_calls_manager.dials_store, &iter, 0, status, -1);
			GtkTreePath *path=gtk_tree_model_get_path(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter);
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),path,NULL,FALSE);
			gtk_tree_path_free(path);
			gui_calls_select_callback();
		}
		g_value_unset(&value);
	}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(g_calls_manager.dials_store), &iter));
	
}

static void gui_calls_utils_add_dial(gchar *dial, gchar *status, SphoneCall *call)
{
	GtkTreeIter iter;
	gui_calls_utils_delete_dial(dial);
	store_contact_struct *contact;
	gchar *desc;
	GdkPixbuf *photo=NULL;
	
	store_contact_match(&contact, dial);
	if(contact && (contact->picture || contact->name)){
		desc=g_strdup_printf("%s\n%s",contact->name, dial);
		if(contact->picture)
			photo=utils_get_photo(contact->picture);
		else
			photo=utils_get_photo_default();
	}else{
		desc=g_strdup_printf("<Unknown>\n%s\n",dial);
		photo=utils_get_photo_unknown();
	}
	store_contact_free(contact);
		
	gtk_list_store_append(g_calls_manager.dials_store, &iter);
	gtk_list_store_set(g_calls_manager.dials_store, &iter, GUI_CALLS_COLUMN_STATUS, status,GUI_CALLS_COLUMN_DIAL , dial, GUI_CALLS_COLUMN_CALL, call, GUI_CALLS_COLUMN_DESC, desc, GUI_CALLS_COLUMN_PHOTO, photo, -1);
	g_object_unref(G_OBJECT(photo));
	g_free(desc);

	GtkTreePath *path=gtk_tree_model_get_path(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),path,NULL,FALSE);
	gtk_tree_path_free(path);
	gtk_widget_show(g_calls_manager.main_window);
}

static void gui_calls_answer_callback()
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value={0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,2,&value);
	SphoneCall *call=g_value_get_object(&value);

	sphone_call_answer(call);
	
	g_value_unset(&value);
}

static void gui_calls_answer_waiting_callback()
{
	sphone_manager_call_hold_and_answer();
}

static void gui_calls_activate_callback()
{
	sphone_manager_call_swap();
}

static void gui_calls_mute_callback()
{
	utils_stop_ringing(NULL);
	gui_calls_update_global_status();
}

static void gui_calls_speaker_callback()
{
	utils_audio_route_set(UTILS_AUDIO_ROUTE_SPEAKER);
	gui_calls_update_global_status();
}

static void gui_calls_handset_callback()
{
	utils_audio_route_set(UTILS_AUDIO_ROUTE_HANDSET);
	gui_calls_update_global_status();
}

static void gui_calls_hangup_callback()
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(GTK_TREE_VIEW(g_calls_manager.dials_view),&path,NULL);
	GtkTreeIter iter;
	GValue value={0};
	gtk_tree_model_get_iter(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,path);
	gtk_tree_path_free(path);
	gtk_tree_model_get_value(GTK_TREE_MODEL(g_calls_manager.dials_store),&iter,2,&value);
	SphoneCall *call=g_value_get_object(&value);

	sphone_call_hangup(call);
	
	g_value_unset(&value);
}
