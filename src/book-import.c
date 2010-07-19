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

int book_import_xml(gchar *text,gsize text_len)
{
	GMarkupParser parser={.start_element=start_element};
	GMarkupParseContext *context=g_markup_parse_context_new(&parser,0,NULL,NULL);
	GError *gerror=NULL;
	gchar *p;
	gboolean ret=TRUE;

	p=text;
	while(*p==0xef || *p==0xbb || *p==0xbf)
		p++,text_len--;
	if(p[0]=='<' && p[1]=='?'){
		while(*p && *p!='>')
			p++,text_len--;
		if(*p=='>' || isspace(*p))p++,text_len--;
	}
	debug("book_import_xml: content=%s\n",p);

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

static gchar* csv_next_token(gchar **p)
{
	gchar *start;
	gboolean isquoted=FALSE;

	while(**p!='\n' && **p && isspace(**p))(*p)++;
	if(**p=='"')isquoted=TRUE, (*p)++;

	start=*p;
	while(**p){
		if(isquoted){
			if(**p=='"'){
				if((*p)[1]=='"')
					(*p)++;
				else
					isquoted=0;
			}
		}else{
			if(**p=='\n' || **p==','){
				break;
			}
			if(**p=='"'){
				isquoted=1;
			}
		}
		(*p)++;
	}

	if(start==*p)
		return NULL;

	gchar *ret=g_new(gchar,*p-start+1);
	gchar *t=ret;
	while(start<*p){
		if(*start=='"' && start[1]=='"'){
			start++;
			*t='"';
			t++;
		}
		else if(*start!='"'){
			*t=*start;
			t++;
		}
		start++;
	}
	*t=0;
	
	while(**p!='\n' && **p && isspace(**p))(*p)++;

	return ret;
}

static int book_import_csv(gchar *text)
{
	int column=0;
	char headers[50];
	char *dials[50];
	int namecolumn=-1;
	int dialcolumns=0;
	char *name;
	int i;
	
	debug ("book_import_csv\n");
	// read header text
	memset(headers,0,sizeof(headers));
	while(*text && *text!='\n'){
		gchar *label=csv_next_token(&text);
		if(label!=NULL){
			if(strcasestr(label,"name"))
				debug("book_import_csv: set name column: %d\n",column),namecolumn=column;
			if(strcasestr(label,"dial") && column<sizeof(headers))
				debug("book_import_csv: add dial column: %d\n",column),dialcolumns++,headers[column]=1;
		}
		if(*text==',')
			text++,column++;
	}
	if(*text=='\n')
		text++;

	if(namecolumn==-1 || !dialcolumns){
		error ("book_import_csv: invalid file format\n");
		return -1;
	}
	
	//read data
	while(*text){
		column=0;
		name=NULL;
		memset(dials,0,sizeof(dials));
		while(*text && *text!='\n'){
			gchar *label=csv_next_token(&text);
			if(label!=NULL){
				if(namecolumn==column)
					name=label;
				else if(column<sizeof(headers) && headers[column])
					dials[column]=label;
				else
					g_free(label);
			}
			if(*text==',')
				text++,column++;
		}
		if(*text=='\n')
			text++;
		if(!name){
			debug ("book_import_csv: ignore line with no name\n");
			for(i=0;i<sizeof(headers);i++)g_free(dials[i]);
			continue;
		}
		for(i=0;i<sizeof(headers);i++){
			if(dials[i]){
				debug ("book_import_csv: add name[%s] dial[%s]\n",name,dials[i]);
			}
		}
		for(i=0;i<sizeof(headers);i++)g_free(dials[i]);
		g_free(name);
	}
	return 0;
}


/*
 Check file format and call the corresponding import method
 */
int book_import(gchar *path)
{
	GError *gerror=NULL;
	gchar *text;
	gsize text_len;
	gboolean ret=TRUE;

	ret=g_file_get_contents(path,&text,&text_len,&gerror);
	if(!ret){
		error("book_import %s\n",gerror->message);
		g_error_free(gerror);
		return 0;
	}

	while(isspace(*text))text++,text_len--;
	if(*text==0)
		goto done;		// empty file
	
	if(*text=='<')
		ret=book_import_xml(text,text_len);
	else
		ret=book_import_csv(text);

done:
	g_free(text);
	return ret;
}
