/*
 * propono-spinner.h
 * This file is part of propono
 *
 * Copyright (C) 2005 - Paolo Maggi 
 * Copyright (C) 2000 - Eazel, Inc. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * This widget was originally written by Andy Hertzfeld <andy@eazel.com> for
 * Caja.
 */

#ifndef __PROPONO_SPINNER_H__
#define __PROPONO_SPINNER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PROPONO_TYPE_SPINNER		(propono_spinner_get_type ())
#define PROPONO_SPINNER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), PROPONO_TYPE_SPINNER, ProponoSpinner))
#define PROPONO_SPINNER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), PROPONO_TYPE_SPINNER, ProponoSpinnerClass))
#define PROPONO_IS_SPINNER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), PROPONO_TYPE_SPINNER))
#define PROPONO_IS_SPINNER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), PROPONO_TYPE_SPINNER))
#define PROPONO_SPINNER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), PROPONO_TYPE_SPINNER, ProponoSpinnerClass))


/* Private structure type */
typedef struct _ProponoSpinnerPrivate	ProponoSpinnerPrivate;

/*
 * Main object structure
 */
typedef struct _ProponoSpinner		ProponoSpinner;

struct _ProponoSpinner
{
	GtkWidget parent;

	/*< private >*/
	ProponoSpinnerPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _ProponoSpinnerClass	ProponoSpinnerClass;

struct _ProponoSpinnerClass
{
	GtkWidgetClass parent_class;
};

/*
 * Public methods
 */
GType		propono_spinner_get_type	(void) G_GNUC_CONST;

GtkWidget      *propono_spinner_new	(void);

void		propono_spinner_start	(ProponoSpinner *throbber);

void		propono_spinner_stop	(ProponoSpinner *throbber);

void		propono_spinner_set_size	(ProponoSpinner *spinner,
					 GtkIconSize   size);

G_END_DECLS

#endif /* __PROPONO_SPINNER_H__ */
