/* cowmail-msg-row.c
 *
 * Copyright 2020 Stephan Verbücheln <verbuecheln@posteo.de>
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cowmail-msg-row.h"

struct _CowmailMsgRow
{
  GtkListBoxRow  parent_instance;

  gchar         *body;
};

G_DEFINE_TYPE (CowmailMsgRow, cowmail_msg_row, GTK_TYPE_LIST_BOX_ROW)



CowmailMsgRow *
cowmail_msg_row_new (const gchar *msg)
{
  CowmailMsgRow *self = COWMAIL_MSG_ROW (g_object_new (COWMAIL_TYPE_MSG_ROW, NULL));

  /* get date and subject line */
  g_autoptr (GDateTime) now = g_date_time_new_now_utc ();
  g_autofree gchar *nowstr = g_date_time_format (now, "%H:%M");
  gchar **lines = g_strsplit (msg, "\n", 2);
  g_autofree gchar *subject = g_strndup (lines[0], 80);
  g_strfreev (lines);

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), gtk_label_new (nowstr), FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (box), gtk_label_new (subject), FALSE, FALSE, 0);
  self->body = g_strdup (msg);
  gtk_container_add (GTK_CONTAINER (self), box);

  return self;
}



gchar *
cowmail_msg_row_get_msg (CowmailMsgRow *self)
{
  return self->body;
}



static void
cowmail_msg_row_finalize (GObject *object)
{
  CowmailMsgRow *self = (CowmailMsgRow *) object;
  COWMAIL_IS_MSG_ROW (self);

  g_free (self->body);

  G_OBJECT_CLASS (cowmail_msg_row_parent_class)->finalize (object);
}



static void
cowmail_msg_row_class_init (CowmailMsgRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cowmail_msg_row_finalize;
}



static void
cowmail_msg_row_init (CowmailMsgRow *self)
{
  self->body = NULL;
}
