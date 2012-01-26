/*
 * propono-ssh.h
 * SSH Utilities for Propono
 * This file is part of propono
 *
 * Copyright (C) 2009 - Jonh Wendell <wendell@bani.com.br>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PROPONO_SSH_H__
#define __PROPONO_SSH_H__

#define PROPONO_SSH_CHECK "ViNagRE_CHEck"
#define PROPONO_SSH_CHECK_LENGTH 13

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum 
{
  PROPONO_SSH_ERROR_FAILED = 1,
  PROPONO_SSH_ERROR_INVALID_CLIENT,
  PROPONO_SSH_ERROR_TIMEOUT,
  PROPONO_SSH_ERROR_PASSWORD_CANCELED,
  PROPONO_SSH_ERROR_IO_ERROR,
  PROPONO_SSH_ERROR_PERMISSION_DENIED,
  PROPONO_SSH_ERROR_HOST_NOT_FOUND
} ProponoSshError;

#define PROPONO_SSH_ERROR propono_ssh_error_quark()
GQuark propono_ssh_error_quark (void);

gboolean propono_ssh_connect (GtkWindow *parent,
			      const gchar *hostname,
			      gint port,
			      const gchar *username,
			      gchar **extra_arguments,
			      gchar **command,
			      gint *tty,
			      GError **error);

G_END_DECLS

#endif  /* __PROPONO_SSH_H__  */
/* vim: set ts=8: */
