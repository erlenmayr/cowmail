/* cowmail-contact-row.c
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

#include "cowmail-contact-row.h"

struct _CowmailContactRow
{
  GtkListBoxRow  parent_instance;

  GtkLabel      *name;

  guchar         pkey[COWMAIL_KEY_SIZE];
};

G_DEFINE_TYPE (CowmailContactRow, cowmail_contact_row, GTK_TYPE_LIST_BOX_ROW)



CowmailContactRow *
cowmail_contact_row_new (cowmail_id *contact)
{
  CowmailContactRow *self = COWMAIL_CONTACT_ROW (g_object_new (COWMAIL_TYPE_CONTACT_ROW, NULL));

  if (contact) {
    gtk_label_set_text (self->name, contact->name);
    memcpy (self->pkey, contact->pkey, COWMAIL_KEY_SIZE);
  } else {
    memset (self->pkey, 0, COWMAIL_KEY_SIZE);
  }

  return self;
}



const gchar *
cowmail_contact_row_get_name (CowmailContactRow *self)
{
  return gtk_label_get_text (self->name);
}



void
cowmail_contact_row_set_name (CowmailContactRow *self,
                              const gchar       *name)
{
  gtk_label_set_text (self->name, name);
}



const guchar *
cowmail_contact_row_get_pkey (CowmailContactRow *self)
{
  return self->pkey;
}



void
cowmail_contact_row_set_pkey (CowmailContactRow *self,
                              const guchar      *pkey)
{
  memcpy (self->pkey, pkey, COWMAIL_KEY_SIZE);
}



static void
cowmail_contact_row_finalize (GObject *object)
{
  CowmailContactRow *self = (CowmailContactRow *) object;
  COWMAIL_IS_CONTACT_ROW (self);

  G_OBJECT_CLASS (cowmail_contact_row_parent_class)->finalize (object);
}



static void
cowmail_contact_row_class_init (CowmailContactRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = cowmail_contact_row_finalize;
}



static void
cowmail_contact_row_init (CowmailContactRow *self)
{
  self->name = GTK_LABEL (gtk_label_new ("(unnamed)"));
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->name));
}
