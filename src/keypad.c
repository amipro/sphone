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
#include "keypad.h"

struct {
	gchar *text;
	gchar *value;
}keys[]={
	{"<span font_size=\"x-large\">1</span>\n<span font_size=\"x-small\">     </span>","1"},
	{" <span font_size=\"x-large\">2</span>\n<span font_size=\"x-small\">ABC</span>","2"},
	{"<span font_size=\"x-large\">3</span>\n<span font_size=\"x-small\">DEF</span>","3"},
	{"<span font_size=\"x-large\">4</span>\n<span font_size=\"x-small\">GHI</span>","4"},
	{"<span font_size=\"x-large\">5</span>\n<span font_size=\"x-small\">JKL</span>","5"},
	{" <span font_size=\"x-large\">6</span>\n<span font_size=\"x-small\">MNO</span>","6"},
	{"<span font_size=\"x-large\"> 7</span>\n<span font_size=\"x-small\">PQRS</span>","7"},
	{" <span font_size=\"x-large\">8</span>\n<span font_size=\"x-small\">TUV</span>","8"},
	{"<span font_size=\"x-large\"> 9</span>\n<span font_size=\"x-small\">WXYZ</span>","9"},
	{"*","*"},
	{"0 +","0"},
	{"#","#"}
};

static void key_presses_callback(GtkWidget *button, GtkWidget *target)
{
	gtk_editable_set_position(GTK_EDITABLE(target),-1);
	gint position=gtk_editable_get_position(GTK_EDITABLE(target));
	gchar *value=g_object_get_data(G_OBJECT(button),"key_value");
	gtk_editable_insert_text(GTK_EDITABLE(target),value,-1, &position);

	gtk_widget_grab_focus(target);
	gtk_editable_set_position(GTK_EDITABLE(target),position);
}

GtkWidget *gui_keypad_setup(GtkWidget *target)
{
	GtkWidget *ret;
	guint row,column;
	guint i=0;

	ret=gtk_table_new (4,3,TRUE);
	for(row=0;row<4;row++)
		for(column=0;column<3;column++){
			GtkWidget *label=gtk_label_new(NULL);
			gtk_label_set_markup (GTK_LABEL(label),keys[i].text);
			GtkWidget *button=gtk_button_new();
			gtk_container_add (GTK_CONTAINER(button),label);
			//gtk_widget_set_can_focus(button,FALSE);
			gtk_table_attach_defaults(GTK_TABLE(ret),button,column,column+1,row,row+1);
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(key_presses_callback), target);
			g_object_set_data(G_OBJECT(button),"key_value",keys[i].value);
			i++;
		}

	return ret;
}
