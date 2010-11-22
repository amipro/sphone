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
#include "utils.h"
#include "store.h"
#include "gui-contact-view.h"
#include "sphone-store-tree-model.h"
#include "gui-sms.h"

static void gui_sms_send_callback(GtkWidget *button, GtkWidget *main_window);
static void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window);
static void gui_sms_reply_callback(GtkWidget *button);

struct{
	SphoneManager *manager;
}g_sms;

static void gui_sms_coming_callback(SphoneManager *manager, gchar *from, gchar *text, gchar *_time)
{
	debug("gui_sms_coming_callback %s %s %s\n",from,text,time);

	gui_sms_receive_show(from,text,_time);
	utils_sms_notify();
}

static void gui_sms_open_contact_callback(GtkButton *button)
{
	gchar *dial=g_object_get_data(G_OBJECT(button),"dial");
	gui_contact_open_by_dial(dial);
}

int gui_sms_init(SphoneManager *manager)
{
	g_sms.manager=manager;
	g_signal_connect (G_OBJECT(manager), "sms_arrived", G_CALLBACK (gui_sms_coming_callback), NULL);

	return 0;
}

// The model will always contain only matched entries
gboolean gui_sms_completion_match(GtkEntryCompletion *completion,const gchar *key,GtkTreeIter *iter,gpointer user_data)
{
	return TRUE;
}

static gint gui_sms_to_changed_callback(GtkEntry *entry)
{
	const gchar *filter=gtk_entry_get_text(entry);
	GtkEntryCompletion *completion=gtk_entry_get_completion(entry);

	SphoneStoreTreeModel *dials_store;
	if(*filter)
		dials_store = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL, filter);
	else
		dials_store = sphone_store_tree_model_new(NULL, NULL);
		
	g_object_set(G_OBJECT(dials_store), "max-rows", 25,NULL);
	gtk_entry_completion_set_model(completion, GTK_TREE_MODEL(dials_store));
	return FALSE;
}

static void gui_sms_to_changed_callback_delayed(GtkEntry *entry)
{
	static int timeout_source_id=-1;
	if(timeout_source_id>-1)
		g_source_remove(timeout_source_id);
	timeout_source_id=g_timeout_add_seconds(1,(GSourceFunc)gui_sms_to_changed_callback, entry);
}

void gui_sms_completion_cell_data(GtkCellLayout *cell_layout,GtkCellRenderer *cell,GtkTreeModel *tree_model,GtkTreeIter *iter,gpointer data)
{
	GValue dial_val={0},name_val={0};
	
	gtk_tree_model_get_value(tree_model,iter,SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&dial_val);
	gtk_tree_model_get_value(tree_model,iter,SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&name_val);
	gchar *content;
	content=g_strdup_printf("%s <%s>",g_value_get_string(&name_val),g_value_get_string(&dial_val));
	g_object_set_data_full(G_OBJECT(cell),"text",content,g_free);
	g_value_unset(&dial_val);
	g_value_unset(&name_val);
}

void gui_sms_send_show(const gchar *to, const gchar *text)
{
	GtkWidget *main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window),"Send SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,2);
	GtkWidget *to_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *actions_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *to_label=gtk_label_new("To:");
	GtkWidget *to_entry=gtk_entry_new();
	GtkWidget *send_button=gtk_button_new_with_label("Send");
	GtkWidget *cancel_button=gtk_button_new_with_label("Cancel");
	GtkWidget *text_edit=gtk_text_view_new();
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GtkTextBuffer *text_buffer=gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_edit));

	if(to)
		gtk_entry_set_text(GTK_ENTRY(to_entry),to);
	if(text)
		gtk_text_buffer_set_text (text_buffer, text, -1);

	GtkCellRenderer *renderer;
	GtkEntryCompletion *completion=gtk_entry_completion_new();
	gtk_entry_completion_set_match_func(completion,gui_sms_completion_match,NULL,NULL);
	gtk_entry_completion_set_popup_completion(completion,TRUE);
	g_object_set(G_OBJECT(completion), "text-column", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,NULL);

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_renderer_set_fixed_size(renderer,40,40);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion),renderer,"pixbuf", SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_fixed_size(renderer,-1,40);
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion),renderer,"text", SPHONE_STORE_TREE_MODEL_COLUMN_NAME);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_renderer_set_fixed_size(renderer,-1,40);
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion),renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(completion),renderer,"text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL);
	
	gtk_entry_set_completion (GTK_ENTRY(to_entry),completion);
	
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit),GTK_TEXT_WINDOW_LEFT,2);
	gtk_text_view_set_border_window_size(GTK_TEXT_VIEW(text_edit),GTK_TEXT_WINDOW_RIGHT,2);

	gtk_box_pack_start(GTK_BOX(to_bar), to_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), to_entry, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(actions_bar), send_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_box_pack_start(GTK_BOX(v1), to_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(s), text_edit);
	gtk_box_pack_start(GTK_BOX(v1), s, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window), v1);

	g_object_set_data(G_OBJECT(main_window),"to_entry",to_entry);
	g_object_set_data(G_OBJECT(main_window),"text_buffer",text_buffer);

	gtk_widget_show_all(main_window);
	
	g_signal_connect(G_OBJECT(send_button),"clicked", G_CALLBACK(gui_sms_send_callback),main_window);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback),main_window);
	g_signal_connect(G_OBJECT(to_entry),"changed", G_CALLBACK(gui_sms_to_changed_callback_delayed),NULL);
	gui_sms_to_changed_callback(GTK_ENTRY(to_entry));
}

void gui_sms_receive_show(const gchar *dial, const gchar *text, gchar *time)
{
	// Get the corresponding contact
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

	GtkWidget *main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window),"New SMS");
	gtk_window_set_default_size(GTK_WINDOW(main_window),400,220);
	GtkWidget *v1=gtk_vbox_new(FALSE,0);
	GtkWidget *to_bar=gtk_hbox_new(FALSE,0);
	GtkWidget *actions_bar=gtk_hbox_new(TRUE,0);
	GtkWidget *photo_image=gtk_image_new_from_pixbuf(photo);
	GtkWidget *from_entry=gtk_button_new_with_label(desc);
	GtkWidget *time_label=gtk_label_new(time);
	GtkWidget *send_button=gtk_button_new_with_label("Reply");
	GtkWidget *cancel_button=gtk_button_new_with_label("Cancel");
	GtkWidget *text_edit=gtk_label_new(text);
	GtkWidget *s = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (s),
		       GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_misc_set_alignment(GTK_MISC(text_edit),0.0,0.0);
	gtk_misc_set_padding(GTK_MISC(text_edit),2,2);
	gtk_button_set_relief(GTK_BUTTON(from_entry),GTK_RELIEF_NONE);
	gtk_button_set_alignment(GTK_BUTTON(from_entry),0,0.5);
	//gtk_widget_set_can_focus(from_entry,FALSE);
	g_object_set(G_OBJECT(from_entry),"can-focus",FALSE,NULL);

	gtk_box_pack_start(GTK_BOX(to_bar), photo_image, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), from_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(to_bar), time_label, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(actions_bar), send_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_box_pack_start(GTK_BOX(v1), to_bar, FALSE, FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(s), text_edit);
	gtk_box_pack_start(GTK_BOX(v1), s, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(v1), actions_bar, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(main_window), v1);

	gtk_widget_show_all(main_window);

	g_object_set_data_full(G_OBJECT(from_entry),"dial",g_strdup(dial),g_free);
	g_object_set_data_full(G_OBJECT(send_button),"dial",g_strdup(dial),g_free);
	
	g_signal_connect(G_OBJECT(from_entry),"clicked", G_CALLBACK(gui_sms_open_contact_callback),NULL);
	g_signal_connect(G_OBJECT(send_button),"clicked", G_CALLBACK(gui_sms_reply_callback),NULL);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_sms_cancel_callback),main_window);
}

void gui_sms_send_callback(GtkWidget *button, GtkWidget *main_window)
{
	GtkEntry *to_entry=g_object_get_data(G_OBJECT(main_window),"to_entry");
	GtkTextBuffer *text_buffer=g_object_get_data(G_OBJECT(main_window),"text_buffer");

	const gchar *to=gtk_entry_get_text(to_entry);
	gchar *text=NULL;
	g_object_get(G_OBJECT(text_buffer),"text",&text,NULL);
	debug("gui_sms_send_callback %s %s\n",to,text);

	if(!sphone_manager_sms_send(g_sms.manager, to, text)){
		gtk_widget_destroy(main_window);	
	}else{
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(main_window),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_CLOSE,
                                  "SMS send action failed");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	g_free(text);
}

void gui_sms_cancel_callback(GtkWidget *button, GtkWidget *main_window)
{
	gtk_widget_destroy(main_window);
}

void gui_sms_reply_callback(GtkWidget *button)
{
	gchar *from=g_object_get_data(G_OBJECT(button),"dial");
	gui_sms_send_show(from, NULL);
}
