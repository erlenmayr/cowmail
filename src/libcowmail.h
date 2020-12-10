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

#define COWMAIL_TAG_SIZE  16
#define COWMAIL_KEY_SIZE  32
#define COWMAIL_HEAD_SIZE 80

#define COWMAIL_DEFAULT_PORT 1337



typedef struct
{
  gchar  *name;
  guchar  pkey[COWMAIL_KEY_SIZE];
  guchar  skey[COWMAIL_KEY_SIZE];
} cowmail_id;



typedef struct
{

  guchar  hash[COWMAIL_KEY_SIZE];
  guchar  secret[COWMAIL_KEY_SIZE];
  guchar  nonce[COWMAIL_TAG_SIZE];
} cowmail_ticket;



/**
 * cowmail_id_new:
 * @name: name for the cowmail identity
 *
 * Allocates a new cowmail identity without keys.
 *
 * Returns: new cowmail identity
 */
cowmail_id        *cowmail_id_new          (const gchar           *name);

/**
 * cowmail_id_generate:
 * @name: name for the cowmail identity
 *
 * Allocates a new cowmail identity with newly generated keys.
 *
 * Returns: new cowmail identity
 */
cowmail_id        *cowmail_id_generate     (const gchar           *name);

/**
 * cowmail_id_from key:
 * @name: name for the cowmail identity
 *
 * Allocates a new cowmail identity based on existing keys.
 *
 * Returns: new cowmail identity
 */
cowmail_id        *cowmail_id_from_key     (const gchar           *name,
                                            const guchar          *pkey,
                                            const guchar          *skey);

/**
 * cowmail_id_free:
 * @id: the cowmail identity to be freed
 *
 * Frees a cowmail identity. Secret keys are set to zero.
 */
void               cowmail_id_free         (cowmail_id            *id);



/**
 * cowmail_ids_store:
 * @file: file to store the identities to
 * @ids: list of identities
 *
 * Stores a list of cowmail identities to a file. Keys are encoded with base64.
 */
void               cowmail_ids_store       (GFile                 *file,
                                            GList                 *ids);

/**
 * cowmail_ids_load:
 * @file: the file to load the cowmail identities from
 *
 * Loads a list of cowmail identities from a file. Lines which cannot be parsed
 * are ignored.
 *
 * Returns: the list of cowmail identities
 */
GList             *cowmail_ids_load        (GFile                 *file);



/**
 * cowmail_put:
 * @server: server to connect to, may include a port (default: 1337)
 * @msg: the message to be put
 * @contact: the recipient's cowmail identity
 *
 * Puts a message for a recipient to a server.
 */
void               cowmail_put             (const gchar           *hostname,
                                            const gchar           *msg,
                                            const cowmail_id      *id);

/**
 * cowmail_list:
 * @server: server to connect to, may include a port (default: 1337)
 * @ids: identities to get messages for
 *
 * Gets all message headers from the server and attempts to decrypt them with
 * the identities. All successfully decrypted headers are put to a list.
 *
 * Returns: the list of headers for the messages
 */
GList             *cowmail_list            (const gchar           *hostname,
                                            const cowmail_id      *id);

/**
 * cowmail_get:
 * @server: server to connect to, may include a port (default: 1337)
 * @h: the header for the message
 *
 * Gets the message for a specific header and decrypts it
 *
 * Returns: the decrypted message
 */
gchar             *cowmail_get             (const gchar           *hostname,
                                            cowmail_ticket        *ticket);



/**
 * cowmail_crypto_test:
 * @id: cowmail identity for test
 *
 * Runs a basic test:
 * - generate an ElGamal key
 * - encrypt a message
 * - decrypt the message, verify auth tag and print result
 */
void               cowmail_crypto_test     (cowmail_id            *id);

/**
 * cowmail_protocol_test:
 * @server: a cowmail server
 * @id: cowmail identity for test
 *
 * Runs a protocol test:
 * - generate an ElGamal key
 * - encrypt a message
 * - send to server
 * - list server messages
 * - receive from server
 * - decrypt the message, verify auth tag and print result
 */
void               cowmail_protocol_test   (const gchar           *server,
                                            cowmail_id            *id);
