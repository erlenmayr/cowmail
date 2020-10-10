/* libcowmail.h
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

#include <gio/gio.h>
#include <sodium/crypto_box.h>



typedef struct
{
  gchar  *name;
  guchar *pkey;
  guchar *skey;
} identity;



identity   *identity_new      (gchar         *name,
                               guchar        *pkey,
                               guchar        *skey);



identity   *identity_generate (gchar         *name);



void        identity_free     (void          *p);



typedef struct
{
  gchar    *hash;
  identity *id;
} head;



head       *head_new          (gchar         *hash,
                               identity      *id);



void        head_free         (void          *p);



void        put_message       (const gchar   *hostname,
                               guint16        port,
                               const gchar   *msg,
                               const gchar   *b64pkey);



gchar      *get_message       (const gchar   *hostname,
                               guint16        port,
                               head          *h);



GSList     *list_heads        (const gchar   *hostname,
                               guint16        port,
                               GSList        *identities);

