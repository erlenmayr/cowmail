/* cowmail-contact-row.h
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

#pragma once

#include <gtk/gtk.h>
#include "libcowmail.h"

G_BEGIN_DECLS

#define COWMAIL_TYPE_CONTACT_ROW (cowmail_contact_row_get_type ())

G_DECLARE_FINAL_TYPE (CowmailContactRow, cowmail_contact_row, COWMAIL, CONTACT_ROW, GtkListBoxRow)

/**
 * cowmail_contact_row_new:
 * @contact: the cowmail contact the row is representing
 *
 * Allocates a new row representing a cowmail contact. If NULL is passed, a new
 * cowmail identity is allocated for a new contact.
 *
 * Returns: newly allocated and initialized contact row
 */
CowmailContactRow *cowmail_contact_row_new       (cowmail_id        *contact);

/**
 * cowmail_contact_row_get_name:
 * @self: the contact row
 *
 * Gets the name of a contact row.
 *
 * Returns: name of the contact
 */
const gchar       *cowmail_contact_row_get_name  (CowmailContactRow *self);

/**
 * cowmail_contact_row_set_name:
 * @self: the contact row
 * @name: the name
 *
 * Sets the name of a contact row.
 */
void               cowmail_contact_row_set_name  (CowmailContactRow *self,
                                                  const gchar       *name);

/**
 * cowmail_contact_row_get_pkey:
 * @self: the contact row
 *
 * Gets the public key of the cowmail contact represented by the row.
 *
 * Returns: public key of the contact
 */
const guchar      *cowmail_contact_row_get_pkey  (CowmailContactRow *self);

/**
 * cowmail_contact_row_set_pkey:
 * @self: the contact row
 * @pkey: the public key
 *
 * Sets the public key of the cowmail contact represented by the row.
 */
void               cowmail_contact_row_set_pkey  (CowmailContactRow *self,
                                                  const guchar      *pkey);

G_END_DECLS
