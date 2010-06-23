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

#ifndef _SPHONE_CALL_H_
#define _SPHONE_CALL_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define SPHONE_TYPE_CALL             (sphone_call_get_type ())
#define SPHONE_CALL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPHONE_TYPE_CALL, SphoneCall))
#define SPHONE_CALL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), SPHONE_TYPE_CALL, SphoneCallClass))
#define SPHONE_IS_CALL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPHONE_TYPE_CALL))
#define SPHONE_IS_CALL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SPHONE_TYPE_CALL))
#define SPHONE_CALL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SPHONE_TYPE_CALL, SphoneCallClass))

typedef struct _SphoneCallClass SphoneCallClass;
typedef struct _SphoneCall SphoneCall;

struct _SphoneCallClass
{
	GObjectClass parent_class;

	/* Signals */
	void (* status_changed) (SphoneCall *self);
};

struct _SphoneCall
{
	GObject parent_instance;
};

GType sphone_call_get_type (void) G_GNUC_CONST;
void sphone_call_answer (SphoneCall *object);
void sphone_call_hangup (SphoneCall *object);

G_END_DECLS

#endif /* _SPHONE_CALL_H_ */
