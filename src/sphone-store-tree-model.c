/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * sphone
 * Copyright (C) Ahmed AbdelHamid 2010 <ahmedam@mail.usa.com>
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
#include <sqlite3.h>
#include "utils.h"
#include "store.h"
#include "sphone-store-tree-model.h"

#define SPHONE_STORE_TREE_MODEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
                                        SPHONE_TYPE_STORE_TREE_MODEL, SphoneStoreTreeModelPrivate))

static void sphone_store_tree_model_class_init (SphoneStoreTreeModelClass *klass);
static void sphone_store_tree_model_tree_model_init (GtkTreeModelIface *iface);
static void sphone_store_tree_model_finalize (GObject *object);
static GtkTreeModelFlags sphone_store_tree_model_get_flags (GtkTreeModel *tree_model);
static gint sphone_store_tree_model_get_n_columns (GtkTreeModel *tree_model);
static GType sphone_store_tree_model_get_column_type (GtkTreeModel *tree_model, gint index);
static gboolean sphone_store_tree_model_get_iter (GtkTreeModel *tree_model,  GtkTreeIter *iter,  GtkTreePath *path);
static GtkTreePath *sphone_store_tree_model_get_path (GtkTreeModel *tree_model,  GtkTreeIter *iter);
static void sphone_store_tree_model_get_value (GtkTreeModel *tree_model,  GtkTreeIter *iter,  gint column,  GValue *value);
static gboolean sphone_store_tree_model_iter_next (GtkTreeModel *tree_model,  GtkTreeIter *iter);
static gboolean sphone_store_tree_model_iter_children (GtkTreeModel *tree_model,  GtkTreeIter *iter,  GtkTreeIter *parent);
static gboolean sphone_store_tree_model_iter_has_child (GtkTreeModel *tree_model,  GtkTreeIter *iter);
static gint sphone_store_tree_model_iter_n_children (GtkTreeModel *tree_model,GtkTreeIter *iter);
static gboolean sphone_store_tree_model_iter_nth_child(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *parent,gint n);
static gboolean sphone_store_tree_model_iter_parent(GtkTreeModel *tree_model,GtkTreeIter *iter,GtkTreeIter *child);

typedef struct{
	int rows_count;
	int head_pos;
	int tail_pos;
	GList *cache;
	int stamp;
	SphoneStoreTreeModelFilter *filter;
	gchar *query;
	gint max_items;
}SphoneStoreTreeModelPrivate;

typedef enum
{
	FILTER_CONTACT,
	FILTER_CALL,
	FILTER_SMS
} FilterType;

typedef enum
{
	FILTER_QUERY_NONE,
	FILTER_QUERY_LIKE,
	FILTER_QUERY_EXACT,
} FilterQueryType;

struct _SphoneStoreTreeModelFilter{
	const gchar *query_count;
	const gchar *query_select;
	FilterType type;
	FilterQueryType query_type;
};

struct SphoneStoreTreeModelCacheEntry{
	gchar *name;
	gchar *dial;
	GdkPixbuf *photo;
	gint interaction_id;
	gchar *interaction_date;
	gchar *interaction_direction;
	gchar *interaction_call_status;
	gchar *interaction_call_duration;
	gchar *interaction_sms_text;
};

enum
{
	PROP_0,

	PROP_FILTER,
	PROP_QUERY,
	PROP_MAX_ITEMS
};


SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL_FUZY={
	.query_count="select count(*) from (select  contact.name, contact.picture, dial.dial, max(interaction.date) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		left join interaction on dial.id=interaction.dial_id where \
		( dial.cleandial like ?1  or name_coded like ?1) group by contact.name, contact.picture, dial.dial)",
	.query_select="select contact.name, contact.picture, dial.dial, max(interaction.date) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		left join interaction on dial.id=interaction.dial_id where \
		( dial.cleandial like ?1  or name_coded like ?1) group by contact.name, contact.picture, dial.dial \
		order by interaction.date desc limit ?2 offset ?3",
	.type=FILTER_CONTACT,
	.query_type=FILTER_QUERY_LIKE,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL={
	.query_count="select count(*) from (select  contact.name, contact.picture, dial.dial, max(interaction.date) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		left join interaction on dial.id=interaction.dial_id where \
		( dial.cleandial like ?1  or name like ?1) group by contact.name, contact.picture, dial.dial)",
	.query_select="select contact.name, contact.picture, dial.dial, max(interaction.date) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		left join interaction on dial.id=interaction.dial_id where \
		( dial.cleandial like ?1  or name like ?1) group by contact.name, contact.picture, dial.dial \
		order by interaction.date desc limit ?2 offset ?3",
	.type=FILTER_CONTACT,
	.query_type=FILTER_QUERY_LIKE,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_CONTACT_ID={
	.query_count="select count(*) from contact \
		join contact_dial on contact.id=contact_dial.contact_id \
		join dial on contact_dial.dial_id=dial.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_call on interaction_call.interaction_id=interaction.id where \
		contact.id=?1 ",
	.query_select="select contact.name, contact.picture, dial.dial, interaction.id, interaction.date, \
		interaction.direction,  interaction_call.status, interaction_call.duration from contact \
		join contact_dial on contact.id=contact_dial.contact_id \
		join dial on contact_dial.dial_id=dial.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_call on interaction_call.interaction_id=interaction.id where \
		contact.id=?1 \
		order by interaction.date desc limit ?2 offset ?3",
	.type=FILTER_CALL,
	.query_type=FILTER_QUERY_EXACT,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_DIAL_ID={
	.query_count="select count(*) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_call on interaction_call.interaction_id=interaction.id \
		where \
		dial.id=?1 ",
	.query_select="select contact.name, contact.picture, dial.dial, interaction.id, interaction.date, \
		interaction.direction,  interaction_call.status, interaction_call.duration from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_call on interaction_call.interaction_id=interaction.id \
		where \
		dial.id=?1 \
		order by interaction.date desc limit ?2 offset ?3",
	.type=FILTER_CALL,
	.query_type=FILTER_QUERY_EXACT,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_CONTACT_ID={
	.query_count="select count(*) from contact \
		join contact_dial on contact.id=contact_dial.contact_id \
		join dial on contact_dial.dial_id=dial.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_sms on interaction_sms.interaction_id=interaction.id \
		where \
		contact.id=?1 ",
	.query_select="select contact.name, contact.picture, dial.dial, interaction.id, interaction.date, \
		interaction.direction,  substr(interaction_sms.content,1,30) from contact \
		join contact_dial on contact.id=contact_dial.contact_id \
		join dial on contact_dial.dial_id=dial.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_sms on interaction_sms.interaction_id=interaction.id \
		where \
		contact.id=?1 \
		order by interaction.date desc limit ?2 offset ?3",
	.type=FILTER_SMS,
	.query_type=FILTER_QUERY_EXACT,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_DIAL_ID={
	.query_count="select count(*) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_sms on interaction_sms.interaction_id=interaction.id \
		where \
		dial.id=?1 ",
	.query_select="select contact.name, contact.picture, dial.dial, interaction.id, interaction.date, \
		interaction.direction,  substr(interaction_sms.content,1,30) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_sms on interaction_sms.interaction_id=interaction.id \
		where \
		dial.id=?1 \
		order by interaction.date desc limit ?2 offset ?3",
	.type=FILTER_SMS,
	.query_type=FILTER_QUERY_EXACT,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_CALLS_ALL={
	.query_count="select count(*) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_call on interaction_call.interaction_id=interaction.id ",
	.query_select="select contact.name, contact.picture, dial.dial, interaction.id, interaction.date, \
		interaction.direction,  interaction_call.status, interaction_call.duration from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_call on interaction_call.interaction_id=interaction.id \
		order by interaction.date desc limit ?1 offset ?2",
	.type=FILTER_CALL,
	.query_type=FILTER_QUERY_NONE,
};

SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_SMS_ALL={
	.query_count="select count(*) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_sms on interaction_sms.interaction_id=interaction.id",
	.query_select="select contact.name, contact.picture, dial.dial, interaction.id, interaction.date, \
		interaction.direction,  substr(interaction_sms.content,1,30) from dial \
		left join contact_dial on dial.id=contact_dial.dial_id \
		left join contact on contact_dial.contact_id=contact.id \
		join interaction on dial.id=interaction.dial_id \
		join interaction_sms on interaction_sms.interaction_id=interaction.id \
		order by interaction.date desc limit ?1 offset ?2",
	.type=FILTER_SMS,
	.query_type=FILTER_QUERY_NONE,
};

static GObjectClass *parent_class = NULL; 

static void
sphone_store_tree_model_init (GObject *object)
{
	SphoneStoreTreeModel *store=SPHONE_STORE_TREE_MODEL(object);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(store);

	if(!private->filter)
		return;
	
	sqlite3_stmt *stmt=NULL;
	
	gchar *query=NULL;
	if(private->filter->query_type==FILTER_QUERY_LIKE)
		query=g_strdup_printf("%%%s%%",private->query);
	
	if(private->filter->query_type==FILTER_QUERY_NONE && store_sql_exec(private->filter->query_count,&stmt,G_TYPE_INVALID)>0){
		private->rows_count=sqlite3_column_int(stmt, 0);
		debug("sphone_store_tree_model_init rows_count=%d\n",private->rows_count);
	}
	else if(store_sql_exec(private->filter->query_count,&stmt,G_TYPE_STRING,query?query:private->query,G_TYPE_INVALID)>0){
		private->rows_count=sqlite3_column_int(stmt, 0);
		debug("sphone_store_tree_model_init rows_count=%d\n",private->rows_count);
	}
	g_free(query);
	if(stmt)sqlite3_finalize(stmt);

	private->cache=NULL;
}

GType
sphone_store_tree_model_get_type (void)
{
	static GType sphone_store_tree_model_type = 0;

	if (sphone_store_tree_model_type == 0)
	{
		static const GTypeInfo sphone_store_tree_model_info =
						{
							sizeof (SphoneStoreTreeModelClass),
							NULL,                                         /* base_init */
							NULL,                                         /* base_finalize */
							(GClassInitFunc) sphone_store_tree_model_class_init,
							NULL,                                         /* class finalize */
							NULL,                                         /* class_data */
							sizeof (SphoneStoreTreeModel),
							0,                                           /* n_preallocs */
							NULL,
							};
							static const GInterfaceInfo tree_model_info =
							{
							(GInterfaceInitFunc)sphone_store_tree_model_tree_model_init,
							NULL,
							NULL
						};

		sphone_store_tree_model_type = g_type_register_static (G_TYPE_OBJECT, "SphoneStoreTreeModel",&sphone_store_tree_model_info, (GTypeFlags)0);
		g_type_add_interface_static (sphone_store_tree_model_type, GTK_TYPE_TREE_MODEL, &tree_model_info);
	}

	return sphone_store_tree_model_type;
}

static void
sphone_store_tree_model_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SPHONE_IS_STORE_TREE_MODEL (object));
	
	SphoneStoreTreeModel *store=SPHONE_STORE_TREE_MODEL(object);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(store);

	switch (prop_id)
	{
	case PROP_FILTER:
		{
			debug("Add filter \n");
			private->filter=g_value_get_pointer(value);			
			private->rows_count=0;
			private->head_pos=0;
			private->tail_pos=0;
			private->cache=NULL;
			private->stamp++;
		}
		break;
	case PROP_QUERY:
		{
			debug("Add query \n");
			private->query=g_value_dup_string(value);			
			private->rows_count=0;
			private->head_pos=0;
			private->tail_pos=0;
			private->cache=NULL;
			private->stamp++;
		}
		break;
	case PROP_MAX_ITEMS:
		private->max_items=g_value_get_int(value);
		if(private->rows_count>private->max_items)
				private->rows_count=private->max_items;
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sphone_store_tree_model_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	g_return_if_fail (SPHONE_IS_STORE_TREE_MODEL (object));

	SphoneStoreTreeModel *store=SPHONE_STORE_TREE_MODEL(object);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(store);

	switch (prop_id)
	{
	case PROP_FILTER:
		g_value_set_pointer(value, private->filter);
		break;
	case PROP_QUERY:
		g_value_set_string(value, private->query);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
sphone_store_tree_model_class_init (SphoneStoreTreeModelClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = sphone_store_tree_model_finalize;
	object_class->set_property = sphone_store_tree_model_set_property;
	object_class->get_property = sphone_store_tree_model_get_property;
	object_class->constructed = sphone_store_tree_model_init;

	g_object_class_install_property (object_class,
	                                 PROP_FILTER,
	                                 g_param_spec_pointer("filter",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_QUERY,
	                                 g_param_spec_string("query",
	                                                      "",
	                                                      "",
	                                                      "",
	                                                      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_MAX_ITEMS,
	                                 g_param_spec_int ("max-rows",
	                                                   "",
	                                                   "",
	                                                   G_MININT, /* TODO: Adjust minimum property value */
	                                                   G_MAXINT, /* TODO: Adjust maximum property value */
	                                                   0,
	                                                   G_PARAM_READWRITE));
	g_type_class_add_private (klass, sizeof (SphoneStoreTreeModelPrivate));
}

static void
sphone_store_tree_model_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags       = sphone_store_tree_model_get_flags;
	iface->get_n_columns   = sphone_store_tree_model_get_n_columns;
	iface->get_column_type = sphone_store_tree_model_get_column_type;
	iface->get_iter        = sphone_store_tree_model_get_iter;
	iface->get_path        = sphone_store_tree_model_get_path;
	iface->get_value       = sphone_store_tree_model_get_value;
	iface->iter_next       = sphone_store_tree_model_iter_next;
	iface->iter_children   = sphone_store_tree_model_iter_children;
	iface->iter_has_child  = sphone_store_tree_model_iter_has_child;
	iface->iter_n_children = sphone_store_tree_model_iter_n_children;
	iface->iter_nth_child  = sphone_store_tree_model_iter_nth_child;
	iface->iter_parent     = sphone_store_tree_model_iter_parent;
}

static GtkTreeModelFlags
sphone_store_tree_model_get_flags (GtkTreeModel *tree_model)
{
	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL(tree_model), (GtkTreeModelFlags)0);
	return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
sphone_store_tree_model_get_n_columns (GtkTreeModel *tree_model)
{
	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL(tree_model), 0);
	return SPHONE_STORE_TREE_MODEL_COLUMN_LAST;
}

static GType
sphone_store_tree_model_get_column_type (GtkTreeModel *tree_model,gint index)
{
	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL(tree_model), G_TYPE_INVALID);

	switch(index){
		case SPHONE_STORE_TREE_MODEL_COLUMN_DIAL:
			return G_TYPE_STRING;
		case SPHONE_STORE_TREE_MODEL_COLUMN_NAME:
			return G_TYPE_STRING;
		case SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE:
			return GDK_TYPE_PIXBUF;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_ID:
			return G_TYPE_INT;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE:
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION:
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_STATUS:
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_DURACTION:
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_SMS_TEXT:
			return G_TYPE_STRING;
		default:
			return G_TYPE_INVALID;
	}
}


static gboolean
sphone_store_tree_model_get_iter (GtkTreeModel *tree_model,GtkTreeIter  *iter,GtkTreePath  *path)
{
	gint          *indices, n, depth;

	g_assert(SPHONE_IS_STORE_TREE_MODEL(tree_model));
	g_assert(path!=NULL);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(tree_model);

	indices = gtk_tree_path_get_indices(path);
	depth   = gtk_tree_path_get_depth(path);

	/* we do not allow children */
	g_assert(depth == 1); /* depth 1 = top level; a list only has top level nodes and no children */

	n = indices[0]; /* the n-th top level row */

	if ( n >= private->rows_count || n < 0 )	
		return FALSE;

	iter->stamp      = 0;
	iter->user_data  = GINT_TO_POINTER(n);
	iter->user_data2 = NULL;   /* unused */
	iter->user_data3 = NULL;   /* unused */

	return TRUE;
}


/*****************************************************************************
*
*  sphone_store_tree_model_get_path: converts a tree iter into a tree path (ie. the
*                        physical position of that row in the list).
*
*****************************************************************************/

static GtkTreePath *
sphone_store_tree_model_get_path (GtkTreeModel *tree_model,GtkTreeIter  *iter)
{
	GtkTreePath  *path;
	glong n;

	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL(tree_model), NULL);
	g_return_val_if_fail (iter != NULL,               NULL);
	g_return_val_if_fail (iter->user_data != NULL,    NULL);

	n = GPOINTER_TO_INT(iter->user_data);

	path = gtk_tree_path_new();
	gtk_tree_path_append_index(path, n);

	return path;
}


/*****************************************************************************
*
*  sphone_store_tree_model_get_value: Returns a row's exported data columns
*                         (_get_value is what gtk_tree_model_get uses)
*
*****************************************************************************/

static void
sphone_store_tree_model_get_value (GtkTreeModel *tree_model,GtkTreeIter  *iter,gint column,GValue *value)
{
	int n;

	g_return_if_fail (SPHONE_IS_STORE_TREE_MODEL (tree_model));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column < SPHONE_STORE_TREE_MODEL_COLUMN_LAST);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(tree_model);

	g_value_init (value, sphone_store_tree_model_get_column_type(tree_model,column));

	n = GPOINTER_TO_INT(iter->user_data);
	if(n >= private->rows_count)
		g_return_if_reached();
//	debug("sphone_store_tree_model_get_value %d\n",n);

	struct SphoneStoreTreeModelCacheEntry *entry=NULL;
	
	// Check if in the cache
	if(n>=private->tail_pos){
		debug("sphone_store_tree_model_get_value: Not in cache, create the cache for 10 more entries\n");
		int count=MAX(private->tail_pos+10, n+10)-private->tail_pos;
		
		sqlite3_stmt *stmt=NULL;
		
		gchar *query=NULL;
		if(private->filter->query_type==FILTER_QUERY_LIKE)
			query=g_strdup_printf("%%%s%%",private->query);
		gint c=0;

		if(private->filter->query_type==FILTER_QUERY_NONE)
			c=store_sql_exec(private->filter->query_select,&stmt, G_TYPE_INT, count, G_TYPE_INT, private->tail_pos, G_TYPE_INVALID)>0;
		else
			c=store_sql_exec(private->filter->query_select,&stmt,G_TYPE_STRING,query?query:private->query, G_TYPE_INT, count, G_TYPE_INT, private->tail_pos, G_TYPE_INVALID)>0;
		
		if(c){
			do{
				entry=g_new0(struct SphoneStoreTreeModelCacheEntry,1);
				entry->name=g_strdup((const gchar *)sqlite3_column_text(stmt,0));
				const gchar *picture=(const gchar *)sqlite3_column_text(stmt,1);
				entry->dial=g_strdup((const gchar *)sqlite3_column_text(stmt,2));
				if(entry->name==NULL && picture==NULL)
					entry->photo=utils_get_photo_unknown();
				else if(picture==NULL)
					entry->photo=utils_get_photo_default();
				else
					entry->photo=utils_get_photo(picture);

				if(private->filter->type==FILTER_CALL || private->filter->type==FILTER_SMS){
					//select contact.name, contact.picture, dial.dial, interaction.date,
					//interaction.direction,  interaction_call.status, interaction_call.duration
					entry->interaction_id=sqlite3_column_int(stmt,3);
					entry->interaction_date=g_strdup((const gchar *)sqlite3_column_text(stmt,4));
					if(sqlite3_column_int(stmt,5)==STORE_INTERACTION_DIRECTION_OUTGOING)
						entry->interaction_direction="Outgoing";
					else
						entry->interaction_direction="Incoming";
				}
				if(private->filter->type==FILTER_SMS){
					entry->interaction_sms_text=g_strdup((const gchar *)sqlite3_column_text(stmt,6));
				}
				if(private->filter->type==FILTER_CALL){
					if(sqlite3_column_int(stmt,6)==STORE_INTERACTION_CALL_STATUS_ESTABLISHED)
						entry->interaction_call_status="Answered";
					else
						entry->interaction_call_status="Missed";
					int n=sqlite3_column_int(stmt,7);
					gchar *duration=g_strdup_printf("%02d:%02d:%02d",n/3600,(n/60)%60,n%60);
					entry->interaction_call_duration=duration;
				}

				//debug("Loaded name=%s dial=%s photo=%s\n",entry->name,entry->dial, picture);
				private->cache=g_list_append(private->cache,entry);
				private->tail_pos++;
			}while(sqlite3_step(stmt)==SQLITE_ROW);
		}
		g_free(query);
		if(stmt)sqlite3_finalize(stmt);
	}

	g_return_if_fail (private->cache != NULL);
	
	entry=(struct SphoneStoreTreeModelCacheEntry *)(g_list_nth(private->cache,n)->data);
	if(!entry){
		error("sphone_store_tree_model_get_value: can't get %d element\n",n);
		return;
		
	}
	switch(column)
	{
		case SPHONE_STORE_TREE_MODEL_COLUMN_NAME:
			if(entry->name)
				g_value_set_string(value, entry->name);
			else
				g_value_set_string(value, "<Unknown>");
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_DIAL:
			g_value_set_string(value, entry->dial);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE:
			g_value_set_object(value, entry->photo);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_ID:
				g_value_set_int(value, entry->interaction_id);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE:
				g_value_set_string(value, entry->interaction_date);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION:
				g_value_set_string(value, entry->interaction_direction);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_STATUS:
				g_value_set_string(value, entry->interaction_call_status);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_DURACTION:
				g_value_set_string(value, entry->interaction_call_duration);
			break;
		case SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_SMS_TEXT:
				g_value_set_string(value, entry->interaction_sms_text);
			break;
	}
}


/*****************************************************************************
*
*  sphone_store_tree_model_iter_next: Takes an iter structure and sets it to point
*                         to the next row.
*
*****************************************************************************/

static gboolean
sphone_store_tree_model_iter_next (GtkTreeModel  *tree_model,GtkTreeIter   *iter)
{
	int n;
	
	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL (tree_model), FALSE);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(tree_model);

	if (iter == NULL)
		return FALSE;

	n = GPOINTER_TO_INT(iter->user_data);
	if ((n + 1) >= private->rows_count)
		return FALSE;

	n++;

	iter->stamp     = 0;
	iter->user_data = GINT_TO_POINTER(n);

	return TRUE;
}


/*****************************************************************************
*
*  sphone_store_tree_model_iter_children: Returns TRUE or FALSE depending on whether
*                             the row specified by 'parent' has any children.
*                             If it has children, then 'iter' is set to
*                             point to the first child. Special case: if
*                             'parent' is NULL, then the first top-level
*                             row should be returned if it exists.
*
*****************************************************************************/

static gboolean
sphone_store_tree_model_iter_children (GtkTreeModel *tree_model,GtkTreeIter  *iter,GtkTreeIter  *parent)
{
	/* this is a list, nodes have no children */
	if (parent)
		return FALSE;

	/* parent == NULL is a special case; we need to return the first top-level row */

	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL (tree_model), FALSE);

	/* Set iter to first item in list */
	iter->stamp     = 0;
	iter->user_data = 0;

	return TRUE;
}


/*****************************************************************************
*
*  sphone_store_tree_model_iter_has_child: Returns TRUE or FALSE depending on whether
*                              the row specified by 'iter' has any children.
*                              We only have a list and thus no children.
*
*****************************************************************************/

static gboolean
sphone_store_tree_model_iter_has_child (GtkTreeModel *tree_model,GtkTreeIter  *iter)
{
	return FALSE;
}


/*****************************************************************************
*
*  sphone_store_tree_model_iter_n_children: Returns the number of children the row
*                               specified by 'iter' has. This is usually 0,
*                               as we only have a list and thus do not have
*                               any children to any rows. A special case is
*                               when 'iter' is NULL, in which case we need
*                               to return the number of top-level nodes,
*                               ie. the number of rows in our list.
*
*****************************************************************************/

static gint
sphone_store_tree_model_iter_n_children (GtkTreeModel *tree_model,GtkTreeIter  *iter)
{
	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL (tree_model), -1);
	g_return_val_if_fail (iter == NULL || iter->user_data != NULL, FALSE);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(tree_model);

	/* special case: if iter == NULL, return number of top-level rows */
	if (!iter){
		return private->rows_count;
	}

	return 0; /* otherwise, this is easy again for a list */
}


/*****************************************************************************
*
*  sphone_store_tree_model_iter_nth_child: If the row specified by 'parent' has any
*                              children, set 'iter' to the n-th child and
*                              return TRUE if it exists, otherwise FALSE.
*                              A special case is when 'parent' is NULL, in
*                              which case we need to set 'iter' to the n-th
*                              row if it exists.
*
*****************************************************************************/

static gboolean
sphone_store_tree_model_iter_nth_child (GtkTreeModel *tree_model,GtkTreeIter  *iter,GtkTreeIter  *parent,gint n)
{
	g_return_val_if_fail (SPHONE_IS_STORE_TREE_MODEL (tree_model), FALSE);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(tree_model);

	/* a list has only top-level rows */
	if(parent)
		return FALSE;

	if( n >= private->rows_count)
		return FALSE;

	iter->stamp = 0;
	iter->user_data = GINT_TO_POINTER(n);

	return TRUE;
}


/*****************************************************************************
*
*  sphone_store_tree_model_iter_parent: Point 'iter' to the parent node of 'child'. As
*                           we have a list and thus no children and no
*                           parents of children, we can just return FALSE.
*
*****************************************************************************/

static gboolean
sphone_store_tree_model_iter_parent (GtkTreeModel *tree_model,GtkTreeIter  *iter,GtkTreeIter  *child)
{
	return FALSE;
}


/*****************************************************************************
*
*  sphone_store_tree_model_new:  This is what you use in your own code to create a
*                    new custom list tree model for you to use.
*
*****************************************************************************/

SphoneStoreTreeModel *
sphone_store_tree_model_new(SphoneStoreTreeModelFilter *filter,const gchar *query)
{
	SphoneStoreTreeModel *newSphoneStoreTreeModel;

	newSphoneStoreTreeModel = (SphoneStoreTreeModel*) g_object_new (SPHONE_TYPE_STORE_TREE_MODEL, "filter", filter,"query",query, NULL);

	g_assert( newSphoneStoreTreeModel != NULL );

	return newSphoneStoreTreeModel;
}

static void
sphone_store_tree_model_finalize (GObject *object)
{
	g_return_if_fail (SPHONE_IS_STORE_TREE_MODEL (object));
	SphoneStoreTreeModel *store=SPHONE_STORE_TREE_MODEL(object);
	SphoneStoreTreeModelPrivate *private=SPHONE_STORE_TREE_MODEL_GET_PRIVATE(store);

	debug("sphone_store_tree_model_finalize\n");
	
	if(private->cache){
		GList *cache=private->cache;
		struct SphoneStoreTreeModelCacheEntry *entry;
		do{
			entry=cache->data;
			g_free(entry->name);
			g_free(entry->dial);
			g_free(entry->interaction_sms_text);
			g_free(entry->interaction_date);
			g_free(entry->interaction_call_duration);
			if(entry->photo)
				g_object_unref(entry->photo);
			g_free(entry);
			cache=g_list_next(cache);
		}while(cache);
		g_list_free(private->cache);
		private->cache=NULL;
	}
	g_free(private->query);

	//(* parent_class->finalize) (object);
}
