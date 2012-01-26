/*
 * propono-ssh-connection.h
 * Child class of abstract ProponoConnection, specific to SSH protocol
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

#ifndef __PROPONO_SSH_CONNECTION_H__
#define __PROPONO_SSH_CONNECTION_H__

#include <propono/propono-connection.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_SSH_CONNECTION             (propono_ssh_connection_get_type ())
#define PROPONO_SSH_CONNECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_SSH_CONNECTION, ProponoSshConnection))
#define PROPONO_SSH_CONNECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_SSH_CONNECTION, ProponoSshConnectionClass))
#define PROPONO_IS_SSH_CONNECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_SSH_CONNECTION))
#define PROPONO_IS_SSH_CONNECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_SSH_CONNECTION))
#define PROPONO_SSH_CONNECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_SSH_CONNECTION, ProponoSshConnectionClass))

typedef struct _ProponoSshConnectionClass   ProponoSshConnectionClass;
typedef struct _ProponoSshConnection        ProponoSshConnection;
typedef struct _ProponoSshConnectionPrivate ProponoSshConnectionPrivate;

struct _ProponoSshConnectionClass
{
  ProponoConnectionClass parent_class;
};

struct _ProponoSshConnection
{
  ProponoConnection parent_instance;
  ProponoSshConnectionPrivate *priv;
};


GType propono_ssh_connection_get_type (void) G_GNUC_CONST;

ProponoConnection*  propono_ssh_connection_new (void);

G_END_DECLS

#endif /* __PROPONO_SSH_CONNECTION_H__  */
/* vim: set ts=8: */
