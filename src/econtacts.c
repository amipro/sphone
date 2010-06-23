/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <libebook/e-book.h>

// Print the dbus error message and free the error object
void error_ebook(GError *error, gchar *domain){
	if(!error)
		return;
	
	g_printerr ("%s: Error: %s\n", domain, error->message);
	
	g_error_free (error);
}

void econtacts_contact_find_callback(EBook *book, EBookStatus status, GList *list, gpointer data)
{
	if(status!=E_BOOK_ERROR_OK){
		debug("econtacts_contact_find_callback failed %d\n", status);
		return;
	}
	debug("econtacts_contact_find_callback has results\n");
	while(list){
		EContact *contact=E_CONTACT(list->data);
		gchar *name=e_contact_get(contact,E_CONTACT_FULL_NAME);
		gchar *phone_mobile=e_contact_get(contact,E_CONTACT_PHONE_MOBILE);
		debug("found name=%s mobile=%s\n",name,phone_mobile);
		list=g_list_next(list);
	}
}
int econtacts_contact_find_by_dial(gchar *dial)
{
	GError *error=NULL;
	int ret=0;

	EBook *ebook=e_book_new_default_addressbook(&error);
	if(error){
		error_ebook(error,"econtatcs_contatc_find_by_dial");
		ret=-1;
		goto error;
	}
	
	if(!e_book_open(ebook,TRUE,&error)){
		error_ebook(error,"econtatcs_contatc_find_by_dial");
		ret=-1;
		goto error;
	}

	EBookQuery *query=e_book_query_from_string(
	                                           "(contains \"mobile_phone\" \"01145\")");
	e_book_async_get_contacts(ebook, query, econtacts_contact_find_callback, NULL);
	
error:
	return ret;
}

void econtacts_test()
{
	econtacts_contact_find_by_dial("01145");
}
