/* cowmail-msg-row.h
 *
 * Copyright 2020 Stephan Verbücheln <verbuecheln@posteo.de#pragma once

#include <glib.h>

G_BEGIN_DECLS



G_END_DECLSt and/or modify
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define COWMAIL_TYPE_MSG_ROW (cowmail_msg_row_get_type ())

G_DECLARE_FINAL_TYPE (CowmailMsgRow, cowmail_msg_row, COWMAIL, MSG_ROW, GtkListBoxRow)

/**
 * cowmail_msg_row_new:
 * @msg: a full message
 *
 * Creates a new message row for a message.
 *
 * Returns: a new message row
 */
CowmailMsgRow *cowmail_msg_row_new     (const gchar   *msg);

/**
 * cowmail_msg_row_get_msg:
 * @self: the message row
 *
 * Gets the full message from a message row.
 *
 * Returns: the full message
 */
gchar         *cowmail_msg_row_get_msg (CowmailMsgRow *self);

G_END_DECLS
