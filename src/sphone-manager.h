/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef _SPHONE_MANAGER_H_
#define _SPHONE_MANAGER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define SPHONE_TYPE_MANAGER             (sphone_manager_get_type ())
#define SPHONE_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPHONE_TYPE_MANAGER, SphoneManager))
#define SPHONE_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SPHONE_TYPE_MANAGER, SphoneManagerClass))
#define SPHONE_IS_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPHONE_TYPE_MANAGER))
#define SPHONE_IS_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SPHONE_TYPE_MANAGER))
#define SPHONE_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SPHONE_TYPE_MANAGER, SphoneManagerClass))

typedef struct _SphoneManagerClass SphoneManagerClass;
typedef struct _SphoneManager SphoneManager;

struct _SphoneManagerClass
{
	GObjectClass parent_class;

	/* Signals */
	void (* calls_change) (SphoneManager *self);
};

struct _SphoneManager
{
	GObject parent_instance;
};

GType sphone_manager_get_type (void) G_GNUC_CONST;
extern void sphone_manager_call_hold_and_answer(void);
extern void sphone_manager_call_swap(void);
extern int sphone_manager_dial(SphoneManager *manager, const gchar *dial);
extern int sphone_manager_sms_send(SphoneManager *manager, const gchar *to, const char *text);
extern void sphone_manager_populate(SphoneManager *manager);

G_END_DECLS

#endif /* _SPHONE_MANAGER_H_ */
