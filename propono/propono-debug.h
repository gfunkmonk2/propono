/*
 * propono-debug.h
 * This file is part of propono
 *
 * Based on pluma code
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002 - 2005 Paolo Maggi
 *
 * propono-debug.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-debug.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_DEBUG_H__
#define __PROPONO_DEBUG_H__

#include <glib.h>

/*
 * Set an environmental var of the same name to turn on
 * debugging output. Setting PROPONO_DEBUG will turn on all
 * sections.
 */
typedef enum {
	PROPONO_NO_DEBUG       = 0,
	PROPONO_DEBUG_VIEW     = 1 << 0,
	PROPONO_DEBUG_PRINT    = 1 << 1,
	PROPONO_DEBUG_PREFS    = 1 << 2,
	PROPONO_DEBUG_PLUGINS  = 1 << 3,
	PROPONO_DEBUG_UTILS    = 1 << 4,
	PROPONO_DEBUG_WINDOW   = 1 << 5,
	PROPONO_DEBUG_LOADER   = 1 << 6,
	PROPONO_DEBUG_APP      = 1 << 7,
	PROPONO_DEBUG_TUBE     = 1 << 8
} ProponoDebugSection;


#define	DEBUG_VIEW	PROPONO_DEBUG_VIEW,    __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PRINT	PROPONO_DEBUG_PRINT,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PREFS	PROPONO_DEBUG_PREFS,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PLUGINS	PROPONO_DEBUG_PLUGINS, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_UTILS	PROPONO_DEBUG_UTILS,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_WINDOW	PROPONO_DEBUG_WINDOW,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LOADER	PROPONO_DEBUG_LOADER,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_APP	PROPONO_DEBUG_APP,     __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_TUBE	PROPONO_DEBUG_TUBE,    __FILE__, __LINE__, G_STRFUNC

void propono_debug_init (void);

void propono_debug (ProponoDebugSection  section,
		    const gchar         *file,
		    gint                 line,
		    const gchar         *function);

void propono_debug_message (ProponoDebugSection  section,
			    const gchar         *file,
			    gint                 line,
			    const gchar         *function,
			    const gchar         *format, ...) G_GNUC_PRINTF(5, 6);


#endif /* __PROPONO_DEBUG_H__ */
/* vim: set ts=8: */
