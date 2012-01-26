/*
 * propono-dirs.h
 * This file is part of propono
 *
 * Copyright (C) 2009 Jorge Pereira <jorge@jorgepereira.com.br>
 * 
 * propono-dirs.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * propono-dirs.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_DIRS_H__
#define __PROPONO_DIRS_H__

#include <glib.h>

G_BEGIN_DECLS

gchar		*propono_dirs_get_user_config_dir		(void);

gchar		*propono_dirs_get_user_cache_dir		(void);

gchar		*propono_dirs_get_user_plugins_dir	(void);

gchar		*propono_dirs_get_user_accels_file	(void);

gchar		*propono_dirs_get_propono_data_dir		(void);

gchar		*propono_dirs_get_propono_locale_dir	(void);

gchar		*propono_dirs_get_propono_lib_dir		(void);

gchar		*propono_dirs_get_propono_plugins_dir	(void);

gchar		*propono_dirs_get_propono_plugin_loaders_dir (void);

gchar		*propono_dirs_get_ui_file			(const gchar *file);

G_END_DECLS

#endif /* __PROPONO_DIRS_H__ */
