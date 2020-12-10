/* cowmail-write-window.h
 *
 * Copyright 2020 Stephan Verbücheln
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gtk/gtk.h>
#include "libcowmail.h"

G_BEGIN_DECLS

#define COWMAIL_TYPE_WRITE_WINDOW (cowmail_write_window_get_type ())

G_DECLARE_FINAL_TYPE (CowmailWriteWindow, cowmail_write_window, COWMAIL, WRITE_WINDOW, GtkWindow)

/**
 * cowmail_write_window_new:
 * @server: server to send message to, may include port (default: 1337)
 * @contacts: potential recipients
 *
 * Allocates a write window for writing a new message.
 *
 * Returns: newly allocated write window
 */
CowmailWriteWindow *cowmail_write_window_new (const gchar *server,
                                              GList       *contacts);

G_END_DECLS
