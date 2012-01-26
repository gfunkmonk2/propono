/*
 * propono-vnc-listener-dialog.h
 * This file is part of propono
 *
 * Copyright (C) 2009 Jonh Wendell <wendell@bani.com.br>
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

#ifndef __PROPONO_VNC_LISTENER_DIALOG_H__
#define __PROPONO_VNC_LISTENER_DIALOG_H__

#include <propono/propono-window.h>
#include <propono/propono-plugin.h>

G_BEGIN_DECLS

void propono_vnc_listener_dialog_show (ProponoWindow *parent, ProponoPlugin *plugin);

G_END_DECLS

#endif /* __PROPONO_VNC_LISTENER_DIALOG_H__ */
/* vim: set ts=8: */
