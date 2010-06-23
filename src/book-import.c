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


#include <ctype.h>
#include <string.h>
#include <glib.h>
#include "utils.h"
#include "store.h"
#include "book-import.h"

static void start_element(GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
	int i;
	const gchar *name=NULL;
	if(!strcmp(element_name,"contact")){
		// Get contact name
		name=NULL;
		for(i=0;attribute_names[i];i++){
			if(!strcmp(attribute_names[i],"fileAs")){
				name=attribute_values[i];
				debug("start_element name=%s\n",name);
				break;
			}
		}

		if(!name)
			return;
		
		// Add dials
		for(i=0;attribute_names[i];i++){
			if(g_strrstr(attribute_names[i],"TelephoneNumber")){
				debug("start_element dial=%s\n",attribute_values[i]);
				store_contact_add_dial(store_contact_add(name),attribute_values[i]);
			}
		}
	}
}

int book_import(gchar *path)
{
	GMarkupParser parser={.start_element=start_element};
	GMarkupParseContext *context=g_markup_parse_context_new(&parser,0,NULL,NULL);
	GError *gerror=NULL;
	gchar *text,*p;
	gsize text_len;
	gboolean ret=TRUE;

	g_file_get_contents(path,&text,&text_len,&gerror);
	if(!ret){
		error("book_import %s\n",gerror->message);
		g_error_free(gerror);
		goto done;
	}

	p=text;
	while(*p==0xef || *p==0xbb || *p==0xbf)
		p++,text_len--;
	if(p[0]=='<' && p[1]=='?'){
		while(*p && *p!='>')
			p++,text_len--;
		if(*p=='>' || isspace(*p))p++,text_len--;
	}
	debug("book_import: content=%s\n",p);

	store_bulk_transaction_start();
	ret=g_markup_parse_context_parse(context,p,text_len,&gerror);
	if(!ret){
		error("book_import %s\n",gerror->message);
		g_error_free(gerror);
		store_transaction_rollback();
		goto done;
	}
	store_transaction_commit();

done:
	g_markup_parse_context_free(context);
	return ret?0:-1;
}
