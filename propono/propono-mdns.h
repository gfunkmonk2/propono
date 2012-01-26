/*
 * propono-mdns.h
 * This file is part of propono
 *
 * Copyright (C) Jonh Wendell 2008,2009 <wendell@bani.com.br>
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PROPONO_MDNS_H_
#define _PROPONO_MDNS_H_

#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS

#define PROPONO_TYPE_MDNS             (propono_mdns_get_type ())
#define PROPONO_MDNS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), PROPONO_TYPE_MDNS, ProponoMdns))
#define PROPONO_MDNS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PROPONO_TYPE_MDNS, ProponoMdnsClass))
#define PROPONO_IS_MDNS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PROPONO_TYPE_MDNS))
#define PROPONO_IS_MDNS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PROPONO_TYPE_MDNS))
#define PROPONO_MDNS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PROPONO_TYPE_MDNS, ProponoMdnsClass))

typedef struct _ProponoMdnsClass   ProponoMdnsClass;
typedef struct _ProponoMdns        ProponoMdns;
typedef struct _ProponoMdnsPrivate ProponoMdnsPrivate;

struct _ProponoMdnsClass
{
  GObjectClass parent_class;

  /* Signals */
  void (* changed) (ProponoMdns *mdns);
};

struct _ProponoMdns
{
  GObject parent_instance;
  ProponoMdnsPrivate *priv;
};

GType		 propono_mdns_get_type    (void) G_GNUC_CONST;

ProponoMdns	*propono_mdns_get_default (void);
GSList		*propono_mdns_get_all     (ProponoMdns *mdns);

G_END_DECLS

#endif /* _PROPONO_MDNS_H_ */
/* vim: set ts=8: */
