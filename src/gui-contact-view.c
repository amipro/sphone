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
#include "utils.h"
#include "store.h"
#include "sphone-manager.h"
#include "gui-sms.h"
#include "gui-dialer.h"
#include "sphone-store-tree-model.h"
#include "gui-contact-view.h"

static void gui_contact_send_sms_callback(GtkButton *button, GtkLabel *dial_label)
{
	const gchar *dial=gtk_label_get_text (dial_label);
	gui_sms_send_show(dial,NULL);
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

static void gui_contact_dial_callback(GtkButton *button, GtkLabel *dial_label)
{
	const gchar *dial=gtk_label_get_text (dial_label);
	gui_dialer_show(dial);
	gtk_widget_destroy(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

static void gui_contact_sms_selected_callback(GtkTreeView *view, GtkLabel *textLabel)
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(view,&path,NULL);
	debug("gui_contact_sms_selected_callback\n");
	if(path){
		GtkTreeModel *model=gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		GValue value={0};
		gtk_tree_model_get_iter(GTK_TREE_MODEL(model),&iter,path);
		gtk_tree_path_free(path);
		gtk_tree_model_get_value(model,&iter,SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_ID,&value);
		gint id=g_value_get_int(&value);
		gchar *content=store_interaction_sms_get_by_id(id);
		gtk_label_set_text(textLabel,content);
		g_free(content);
		g_value_unset(&value);
	}
}

static void gui_contact_book_double_click_callback(GtkTreeView *view, gpointer func_data)
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(view,&path,NULL);
	debug("gui_contact_book_double_click_callback\n");
	if(path){
		GtkTreeModel *model=gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		GValue value={0};
		gtk_tree_model_get_iter(GTK_TREE_MODEL(model),&iter,path);
		gtk_tree_path_free(path);
		gtk_tree_model_get_value(model,&iter,SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&value);
		const gchar *dial=g_value_get_string(&value);
		gui_contact_open_by_dial(dial);
		g_value_unset(&value);
	}
}

static gboolean gui_contact_make_null(GtkWidget *w,GdkEvent *event,GtkWidget **window)
{
	*window=NULL;
	return FALSE;
}

gint gui_contact_open_by_dial(const gchar *dial)
{
	debug("gui_contact_open_by_dial\n");
	store_contact_struct *contact=NULL;
	store_contact_match(&contact,dial);
	if(!contact)
		return -1;

	store_contact_load_details(contact);

	GtkWidget *window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	if(contact->name)
		gtk_window_set_title(GTK_WINDOW(window),contact->name);
	else
		gtk_window_set_title(GTK_WINDOW(window),"<Unknown>");
	gtk_window_set_default_size(GTK_WINDOW(window),400,220);
	GtkWidget *notebook=gtk_notebook_new();
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *title_bar=gtk_hbox_new(FALSE,0);
	GdkPixbuf *pixbuf=NULL;
	if(contact->name==NULL && contact->picture==NULL)
		pixbuf=utils_get_photo_unknown();
	else if(contact->picture==NULL)
		pixbuf=utils_get_photo_default();
	else
		pixbuf=utils_get_photo(contact->picture);
	GtkWidget *picture=gtk_image_new_from_pixbuf(pixbuf);
	GtkWidget *name;
	if(contact->name){
		name=gtk_label_new (contact->name);
	}else{
		name=gtk_label_new ("<Unknown>");
	}

	gtk_box_pack_start(GTK_BOX(title_bar), picture, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(title_bar), name, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v1), title_bar, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(v1), gtk_hseparator_new(), FALSE, FALSE, 5);

	GList *dials=contact->dials;
	debug("Dials %p\n",dials);
	while(dials){
		gchar *dial=((store_dial_struct*)dials->data)->dial;
		GtkWidget *label=gtk_label_new(dial);
		GtkWidget *button_sms=gtk_button_new ();
		GtkWidget *button_voice=gtk_button_new ();
		GtkWidget *h_dial=gtk_hbox_new(FALSE,0);

		GtkWidget *sms=gtk_image_new_from_pixbuf(utils_get_icon("sms.png"));
		GtkWidget *voice=gtk_image_new_from_pixbuf(utils_get_icon("voice.png"));

		g_signal_connect(G_OBJECT(button_sms),"clicked", G_CALLBACK(gui_contact_send_sms_callback),label);
		g_signal_connect(G_OBJECT(button_voice),"clicked", G_CALLBACK(gui_contact_dial_callback),label);

		gtk_container_add (GTK_CONTAINER(button_sms),sms);
		gtk_container_add (GTK_CONTAINER(button_voice),voice);
		
		gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
		debug("gui_contact_open_by_dial add dial %d\n",dial);
		gtk_box_pack_start(GTK_BOX(h_dial), label, TRUE, TRUE, 20);
		gtk_box_pack_start(GTK_BOX(h_dial), button_sms, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(h_dial), button_voice, FALSE, FALSE, 5);
		gtk_box_pack_start(GTK_BOX(v1), h_dial, FALSE, FALSE, 0);
		dials=g_list_next(dials);
	}

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),v1,gtk_label_new("Details"));

	// Call history
	GtkWidget *scroll;
	GtkWidget *calls_view = gtk_tree_view_new();
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(calls_view),TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(calls_view),TRUE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Dir", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Status", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_STATUS, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE, NULL);
	gtk_tree_view_column_set_fixed_width(column,80);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Duration", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_DURACTION, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(calls_view),TRUE);
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER(scroll),calls_view);

	//g_signal_connect_after(G_OBJECT(g_gui_calls.dials_view),"cursor-changed", G_CALLBACK(gui_dialer_book_click_callback),NULL);

	SphoneStoreTreeModel *calls;
	if(contact->name!=NULL){		// known contact
		gchar contactid[20];
		sprintf(contactid,"%d",contact->id);
		debug("SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_CONTACT_ID query='%s'\n",contactid);
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_CONTACT_ID, contactid);
	}else{
		gchar dialid[20];
		sprintf(dialid,"%d",store_dial_get_id(dial));
		debug("SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_DIAL_ID query='%s'\n",dialid);
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_DIAL_ID, dialid);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(calls_view), GTK_TREE_MODEL(calls));
	g_object_unref(G_OBJECT(calls));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scroll,gtk_label_new("Calls"));

	// SMS history
	v1=gtk_vbox_new(FALSE,0);
	calls_view = gtk_tree_view_new();
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(calls_view),TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(calls_view),TRUE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Dir", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE, NULL);
	gtk_tree_view_column_set_fixed_width(column,80);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Text", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_SMS_TEXT, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(calls_view),TRUE);
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER(scroll),calls_view);

	GtkWidget *sms_content=gtk_label_new (NULL);

	g_signal_connect_after(G_OBJECT(calls_view),"cursor-changed", G_CALLBACK(gui_contact_sms_selected_callback),sms_content);

	if(contact->name!=NULL){		// known contact
		gchar contactid[20];
		sprintf(contactid,"%d",contact->id);
		debug("SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_CONTACT_ID query='%s'\n",contactid);
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_CONTACT_ID, contactid);
	}else{
		gchar dialid[20];
		sprintf(dialid,"%d",store_dial_get_id(dial));
		debug("SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_DIAL_ID query='%s'\n",dialid);
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_DIAL_ID, dialid);
	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(calls_view), GTK_TREE_MODEL(calls));
	g_object_unref(G_OBJECT(calls));

	gtk_container_add (GTK_CONTAINER(v1),scroll);
	gtk_container_add (GTK_CONTAINER(v1),sms_content);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),v1,gtk_label_new("Messages"));

	gtk_container_add(GTK_CONTAINER(window),notebook);
	gtk_widget_show_all(window);

	store_contact_free(contact);
	return 0;
}

static GtkWidget *calls_window=NULL;
gint gui_history_calls(void)
{
	debug("gui_history_calls\n");
	SphoneStoreTreeModel *calls;
	static GtkWidget *calls_view=NULL;

	if(calls_window){
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_CALLS_ALL, NULL);
		gtk_tree_view_set_model(GTK_TREE_VIEW(calls_view), GTK_TREE_MODEL(calls));
		g_object_unref(G_OBJECT(calls));
		gtk_window_present(GTK_WINDOW(calls_window));
		return 0;
	}

	calls_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(calls_window),"Call History");
	gtk_window_set_default_size(GTK_WINDOW(calls_window),600,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	// Call history
	GtkWidget *scroll;
	calls_view = gtk_tree_view_new();
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(calls_view),TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(calls_view),TRUE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "pixbuf", SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE, NULL);
	gtk_tree_view_column_set_fixed_width(column,40);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Dir", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Status", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_STATUS, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_NAME, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE, NULL);
	gtk_tree_view_column_set_fixed_width(column,80);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Duration", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_DURACTION, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(calls_view),TRUE);
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER(scroll),calls_view);
	gtk_container_add (GTK_CONTAINER(v1),scroll);
	gtk_container_add (GTK_CONTAINER(calls_window),v1);

	g_signal_connect_after(G_OBJECT(calls_view),"row-activated", G_CALLBACK(gui_contact_book_double_click_callback),NULL);
	g_signal_connect(G_OBJECT(calls_window),"delete-event", G_CALLBACK(gui_contact_make_null),&calls_window);

	calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_CALLS_ALL, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(calls_view), GTK_TREE_MODEL(calls));
	g_object_unref(G_OBJECT(calls));
	
	gtk_widget_show_all(calls_window);

	return 0;
}

static GtkWidget *sms_window=NULL;
gint gui_history_sms(void)
{
	SphoneStoreTreeModel *calls;
	static GtkWidget *calls_view=NULL;
	debug("gui_history_sms\n");

	if(sms_window){
		calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_SMS_ALL, NULL);
		gtk_tree_view_set_model(GTK_TREE_VIEW(calls_view), GTK_TREE_MODEL(calls));
		g_object_unref(G_OBJECT(calls));
		gtk_window_present(GTK_WINDOW(sms_window));
		return 0;
	}
	sms_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(sms_window),"SMS History");
	gtk_window_set_default_size(GTK_WINDOW(sms_window),600,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *scroll;
	calls_view = gtk_tree_view_new();
	gtk_tree_view_set_show_expanders(GTK_TREE_VIEW(calls_view),TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(calls_view),TRUE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("", renderer, "pixbuf", SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE, NULL);
	gtk_tree_view_column_set_fixed_width(column,40);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Dir", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION, NULL);
	gtk_tree_view_column_set_fixed_width(column,70);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_NAME, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("Date", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE, NULL);
	gtk_tree_view_column_set_fixed_width(column,80);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Text", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_SMS_TEXT, NULL);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column(GTK_TREE_VIEW(calls_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(calls_view),TRUE);
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	GtkWidget *sms_content=gtk_label_new (NULL);
	g_signal_connect_after(G_OBJECT(calls_view),"cursor-changed", G_CALLBACK(gui_contact_sms_selected_callback),sms_content);
	g_signal_connect_after(G_OBJECT(calls_view),"row-activated", G_CALLBACK(gui_contact_book_double_click_callback),NULL);

	gtk_container_add (GTK_CONTAINER(scroll),calls_view);
	gtk_container_add (GTK_CONTAINER(v1),scroll);
	gtk_container_add (GTK_CONTAINER(v1),sms_content);
	gtk_container_add (GTK_CONTAINER(sms_window),v1);

	g_signal_connect(G_OBJECT(sms_window),"delete-event", G_CALLBACK(gui_contact_make_null),&sms_window);

	calls = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_SMS_ALL, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(calls_view), GTK_TREE_MODEL(calls));
	g_object_unref(G_OBJECT(calls));
	
	gtk_widget_show_all(sms_window);

	return 0;
}
