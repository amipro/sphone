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

#ifndef _SPHONE_STORE_TREE_MODEL_H_
#define _SPHONE_STORE_TREE_MODEL_H_

G_BEGIN_DECLS

#define SPHONE_TYPE_STORE_TREE_MODEL             (sphone_store_tree_model_get_type ())
#define SPHONE_STORE_TREE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPHONE_TYPE_STORE_TREE_MODEL, SphoneStoreTreeModel))
#define SPHONE_STORE_TREE_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SPHONE_TYPE_STORE_TREE_MODEL, SphoneStoreTreeModelClass))
#define SPHONE_IS_STORE_TREE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPHONE_TYPE_STORE_TREE_MODEL))
#define SPHONE_IS_STORE_TREE_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SPHONE_TYPE_STORE_TREE_MODEL))
#define SPHONE_STORE_TREE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SPHONE_TYPE_STORE_TREE_MODEL, SphoneStoreTreeModelClass))

typedef struct _SphoneStoreTreeModelClass SphoneStoreTreeModelClass;
typedef struct _SphoneStoreTreeModel SphoneStoreTreeModel;
typedef struct _SphoneStoreTreeModelFilter SphoneStoreTreeModelFilter;

enum {
	SPHONE_STORE_TREE_MODEL_COLUMN_NAME,
	SPHONE_STORE_TREE_MODEL_COLUMN_DIAL,
	SPHONE_STORE_TREE_MODEL_COLUMN_PICTURE,
	SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_ID,
	SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DATE,
	SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_DIRECTION,
	SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_STATUS,
	SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_CALL_DURACTION,
	SPHONE_STORE_TREE_MODEL_COLUMN_INTERACTION_SMS_TEXT,
	SPHONE_STORE_TREE_MODEL_COLUMN_LAST
};

struct _SphoneStoreTreeModelClass
{
	GObjectClass parent_class;
};

struct _SphoneStoreTreeModel
{
	GObject parent_instance;
};

GType sphone_store_tree_model_get_type (void) G_GNUC_CONST;

extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL_FUZY;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_MATCH_NAME_DIAL;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_CONTACT_ID;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_CALLS_MATCH_DIAL_ID;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_CONTACT_ID;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_SMS_MATCH_DIAL_ID;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_CALLS_ALL;
extern SphoneStoreTreeModelFilter SPHONE_STORE_TREE_MODEL_FILTER_SMS_ALL;

SphoneStoreTreeModel *sphone_store_tree_model_new(SphoneStoreTreeModelFilter *filter, const gchar *query);

G_END_DECLS

#endif /* _SPHONE_STORE_TREE_MODEL_H_ */
