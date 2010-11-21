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
#include <ctype.h>
#include <gtk/gtk.h>
#include "sphone-manager.h"
#include "keypad.h"
#include "utils.h"
#include "keys-grab.h"
#include "gui-dialer.h"
#include "gui-contact-view.h"
#include "sphone-store-tree-model.h"

struct{
	SphoneManager *manager;
	GtkWidget *display;
	GtkWidget *main_window;
	GtkWidget *dials_view;
}g_gui_calls;

static int gui_dialer_book_update_model()
{
	debug("gui_dialer_book_update_model\n");
	gboolean isvisible;
	g_object_get(G_OBJECT(g_gui_calls.main_window),"visible",&isvisible,NULL);
	//if(!gtk_widget_get_visible(g_gui_calls.main_window))
	if(!isvisible)
		return FALSE;
	const gchar *dial=gtk_entry_get_text(GTK_ENTRY(g_gui_calls.display));
	
	SphoneStoreTreeModel *dials_store = sphone_store_tree_model_new(&SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL_FUZY, dial);

	gtk_tree_view_set_model(GTK_TREE_VIEW(g_gui_calls.dials_view), GTK_TREE_MODEL(dials_store));
	g_object_unref(G_OBJECT(dials_store));

	return FALSE;
}

static int gui_dialer_book_timeout_source_id=-1;
static void gui_dialer_book_update_model_delayed()
{
	if(gui_dialer_book_timeout_source_id>-1)
		g_source_remove(gui_dialer_book_timeout_source_id);
	gui_dialer_book_timeout_source_id=g_timeout_add_seconds(1,(GSourceFunc)gui_dialer_book_update_model, NULL);
}

static void gui_dialer_book_delete_model()
{
	gtk_tree_view_set_model(GTK_TREE_VIEW(g_gui_calls.dials_view), NULL);
}

static void gui_call_callback(GtkButton button)
{
	const gchar *dial=gtk_entry_get_text(GTK_ENTRY(g_gui_calls.display));
	if(!sphone_manager_dial(g_gui_calls.manager,dial)){
		gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display),"");
		gtk_widget_hide(g_gui_calls.main_window);
	}else{
		GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(g_gui_calls.main_window),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_CLOSE,
                                  "Dialing failed");
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		
		gtk_widget_grab_focus(g_gui_calls.display);
		gtk_editable_set_position(GTK_EDITABLE(g_gui_calls.display),-1);
	}
}

static void gui_dialer_cancel_callback()
{
	gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display),"");
	gtk_widget_hide(g_gui_calls.main_window);
}

static void gui_dialer_back_presses_callback(GtkWidget *button, GtkWidget *target)
{
	gtk_editable_set_position(GTK_EDITABLE(target),-1);
	gint position=gtk_editable_get_position(GTK_EDITABLE(target));
	gtk_editable_delete_text (GTK_EDITABLE(target),position-1, position);

	gtk_editable_set_position(GTK_EDITABLE(target),position);
}

static void gui_dialer_book_focus_callback()
{
	gtk_widget_grab_focus(g_gui_calls.display);
	gtk_editable_set_position(GTK_EDITABLE(g_gui_calls.display),-1);
}

static void gui_dialer_book_click_callback(GtkTreeView *view, gpointer func_data)
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(view,&path,NULL);
	debug("gui_dialer_book_click_callback\n");
	if(path){
		GtkTreeModel *model=gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		GValue value={0};
		gtk_tree_model_get_iter(GTK_TREE_MODEL(model),&iter,path);
		gtk_tree_path_free(path);
		gtk_tree_model_get_value(model,&iter,SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&value);
		const gchar *dial=g_value_get_string(&value);
		gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display),dial);
		g_value_unset(&value);
	}
}

static void gui_dialer_book_double_click_callback(GtkTreeView *view, gpointer func_data)
{
	GtkTreePath *path;
	gtk_tree_view_get_cursor(view,&path,NULL);
	debug("gui_dialer_book_double_click_callback\n");
	if(path){
		GtkTreeModel *model=gtk_tree_view_get_model (view);
		GtkTreeIter iter;
		GValue value={0};
		gtk_tree_model_get_iter(GTK_TREE_MODEL(model),&iter,path);
		gtk_tree_path_free(path);
		gtk_tree_model_get_value(model,&iter,SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,&value);
		const gchar *dial=g_value_get_string(&value);
		if(!gui_contact_open_by_dial(dial))
			gui_dialer_cancel_callback();
		g_value_unset(&value);
	}
}

GtkWidget *gui_dialer_build_book()
{
	GtkWidget *scroll;
	g_gui_calls.dials_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_gui_calls.dials_view),FALSE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes("Photo", renderer, "pixbuf", SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column,40);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_gui_calls.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_NAME, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_gui_calls.dials_view), column);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes("Dial", renderer, "text", SPHONE_STORE_TREE_MODEL_COLUMN_DIAL, NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_expand(column,TRUE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_gui_calls.dials_view), column);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(g_gui_calls.dials_view),TRUE);
	scroll = gtk_scrolled_window_new(NULL,NULL);
	gtk_container_add (GTK_CONTAINER(scroll),g_gui_calls.dials_view);

	g_signal_connect(G_OBJECT(g_gui_calls.dials_view),"focus-in-event", G_CALLBACK(gui_dialer_book_focus_callback),NULL);
	g_signal_connect_after(G_OBJECT(g_gui_calls.dials_view),"cursor-changed", G_CALLBACK(gui_dialer_book_click_callback),NULL);
	g_signal_connect_after(G_OBJECT(g_gui_calls.dials_view),"row-activated", G_CALLBACK(gui_dialer_book_double_click_callback),NULL);
	g_signal_connect(G_OBJECT(g_gui_calls.main_window),"unmap-event", G_CALLBACK(gui_dialer_book_delete_model),NULL);
	g_signal_connect(G_OBJECT(g_gui_calls.display),"changed", G_CALLBACK(gui_dialer_book_update_model_delayed),NULL);

	return scroll;
}

static void gui_dialer_validate_callback(GtkEntry *entry,const gchar *text,gint length,gint *position,gpointer data)
{
	int i, count=0;
	gchar *result = g_new (gchar, length);

	for (i=0; i < length; i++) {
		if (!isdigit(text[i]) && text[i]!='*' && text[i]!='#'  && text[i]!='+')
			continue;
		result[count++] = text[i];
	}

	if (count > 0) {
		GtkEditable *editable=GTK_EDITABLE(entry);
		g_signal_handlers_block_by_func (G_OBJECT (editable),	G_CALLBACK (gui_dialer_validate_callback),data);
		gtk_editable_insert_text (editable, result, count, position);
		g_signal_handlers_unblock_by_func (G_OBJECT (editable),	G_CALLBACK (gui_dialer_validate_callback),data);
	}
	g_signal_stop_emission_by_name (G_OBJECT(entry), "insert_text");

	g_free (result);
}

int gui_dialer_init(SphoneManager *manager)
{
	g_gui_calls.main_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(g_gui_calls.main_window),"Dialer");
	gtk_window_set_deletable(GTK_WINDOW(g_gui_calls.main_window),FALSE);
	gtk_window_set_default_size(GTK_WINDOW(g_gui_calls.main_window),400,220);
	gtk_window_maximize(GTK_WINDOW(g_gui_calls.main_window));
	GtkWidget *h1=gtk_hpaned_new();
	GtkWidget *v1=gtk_table_new (6,2,TRUE);
	GtkWidget *actions_bar=gtk_hbox_new(TRUE,0);
	GtkWidget *display_back=gtk_button_new_with_label ("    <    ");
	GtkWidget *display=gtk_entry_new();
	GtkWidget *display_bar=gtk_hbox_new(FALSE,4);
	GtkWidget *keypad=gui_keypad_setup(display);
	GtkWidget *call_button=gtk_button_new_with_label("Call");
	GtkWidget *cancel_button=gtk_button_new_with_label("Cancel");
	GdkColor white, black;
	GtkWidget *e=gtk_event_box_new ();
	g_gui_calls.manager=manager;
	g_gui_calls.display=display;
	GtkWidget *book=gui_dialer_build_book();

	gtk_widget_modify_font(display,pango_font_description_from_string("Monospace 24"));
	gtk_entry_set_alignment(GTK_ENTRY(display),1.0);
	gtk_entry_set_has_frame(GTK_ENTRY(display),FALSE);
		
	gdk_color_parse ("black",&black);
	gdk_color_parse ("white",&white);
	
	gtk_widget_modify_text(display,GTK_STATE_NORMAL,&white);
	gtk_widget_modify_base(display,GTK_STATE_NORMAL,&black);
	gtk_widget_modify_bg(e,GTK_STATE_NORMAL,&black);
	
	gtk_container_add(GTK_CONTAINER(e), display);
	gtk_box_pack_start(GTK_BOX(display_bar), e, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(display_bar), display_back, FALSE, FALSE, 0);
	gtk_table_attach_defaults (GTK_TABLE(v1),display_bar,0,2,0,1);
	gtk_table_attach_defaults (GTK_TABLE(v1),keypad,0,2,1,5);
	gtk_container_add(GTK_CONTAINER(actions_bar), call_button);
	gtk_container_add(GTK_CONTAINER(actions_bar), cancel_button);
	gtk_table_attach_defaults (GTK_TABLE(v1),actions_bar,0,2,5,6);
	gtk_paned_pack1(GTK_PANED(h1), v1, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(h1), book, TRUE, TRUE);
	gtk_container_add(GTK_CONTAINER(g_gui_calls.main_window), h1);

	gtk_widget_show_all(h1);
	gtk_widget_grab_focus(display);
	
	g_signal_connect(G_OBJECT(call_button),"clicked", G_CALLBACK(gui_call_callback),NULL);
	g_signal_connect(G_OBJECT(cancel_button),"clicked", G_CALLBACK(gui_dialer_cancel_callback),NULL);
	g_signal_connect(G_OBJECT(g_gui_calls.main_window),"delete-event", G_CALLBACK(gtk_widget_hide_on_delete),NULL);
	g_signal_connect(G_OBJECT(display_back), "clicked", G_CALLBACK(gui_dialer_back_presses_callback), display);
	g_signal_connect(G_OBJECT(display), "insert_text", G_CALLBACK(gui_dialer_validate_callback),NULL);

	return 0;
}

void gui_dialer_show(const gchar *dial)
{
	gtk_window_present(GTK_WINDOW(g_gui_calls.main_window));
	gtk_widget_grab_focus(g_gui_calls.display);

	gui_dialer_book_update_model();
	if(dial)
		gtk_entry_set_text(GTK_ENTRY(g_gui_calls.display),dial);
}
