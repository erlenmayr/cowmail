/* libcowmail.c
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

#include "libcowmail.h"
#include <gnutls/crypto.h>
#include <gnutls/abstract.h>
#include <nettle/curve25519.h>
#include <nettle/memops.h>
#include <nettle/gcm.h>



cowmail_id *
cowmail_id_new (const gchar *name)
{
  cowmail_id *id = g_malloc0 (sizeof (cowmail_id));
  id->name = g_strdup (name);
  return id;
}



cowmail_id *
cowmail_id_generate (const gchar *name)
{
  cowmail_id *id = cowmail_id_new (name);
  gnutls_rnd (GNUTLS_RND_KEY, id->skey, CURVE25519_SIZE);
  curve25519_mul_g (id->pkey, id->skey);
  return id;
}



cowmail_id *
cowmail_id_from_key (const gchar  *name,
                     const guchar *pkey,
                     const guchar *skey)
{
  cowmail_id *id = cowmail_id_new (name);
  memcpy (id->pkey, pkey, COWMAIL_KEY_SIZE);
  if (skey)
    memcpy (id->skey, skey, COWMAIL_KEY_SIZE);
  return id;
}



void
cowmail_id_free (cowmail_id *id)
{
  if (id->name)
    g_free (id->name);
  memset (id->pkey, 0, COWMAIL_KEY_SIZE);
  memset (id->skey, 0, COWMAIL_KEY_SIZE);
  g_free (id);
}



void
cowmail_ids_store (GFile *file,
                   GList *ids)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GFileOutputStream) ostream = g_file_replace (file, NULL, TRUE, G_FILE_CREATE_NONE, NULL, &error);
  g_autoptr (GDataOutputStream) dstream = g_data_output_stream_new (G_OUTPUT_STREAM (ostream));
  for (GList *idl = ids; idl; idl = idl->next) {
    cowmail_id *id = ((cowmail_id *) idl->data);
    g_data_output_stream_put_string (dstream, id->name, NULL, &error);
    g_data_output_stream_put_byte (dstream, ' ', NULL, &error);

    g_autofree gchar *pkey = g_base64_encode (id->pkey, COWMAIL_KEY_SIZE);
    g_data_output_stream_put_string (dstream, pkey, NULL, &error);
    g_data_output_stream_put_byte (dstream, ' ', NULL, &error);

    g_autofree gchar *skey = g_base64_encode (id->skey, COWMAIL_KEY_SIZE);
    g_data_output_stream_put_string (dstream, skey, NULL, &error);
    g_data_output_stream_put_byte (dstream, '\n', NULL, &error);
  }
  g_output_stream_close (G_OUTPUT_STREAM (ostream), NULL, &error);
}



GList *
cowmail_ids_load (GFile *file)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GFileInputStream) istream = g_file_read (file, NULL, &error);
  if (!istream) {
    g_print ("COWMAIL ERROR: %s\n", error->message);
    return NULL;
  }
  g_autoptr (GDataInputStream) dstream = g_data_input_stream_new (G_INPUT_STREAM (istream));

  GList *ids = NULL;
  gchar *line = NULL;
  while ((line = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, &error))) {
    gchar **e = g_strsplit_set (line, " \n", 4);
    if (e[0] && e[1] && e[2]) {
      gsize plen;
      g_autofree guchar *pkey = g_base64_decode (e[1], &plen);
      gsize slen;
      g_autofree guchar *skey = g_base64_decode (e[2], &slen);
      if (slen == COWMAIL_KEY_SIZE && slen == COWMAIL_KEY_SIZE) {
        cowmail_id *id = cowmail_id_from_key (e[0], pkey, skey);
        ids = g_list_prepend (ids, id);
      } else {
        g_autofree gchar *fname = g_file_get_basename (file);
        g_printerr ("COWMAIL ERROR: Invalid key in file: %s\n", fname);
      }
      memset (skey, 0, COWMAIL_KEY_SIZE);
    } else {
      g_autofree gchar *fname = g_file_get_basename (file);
      g_printerr ("COWMAIL ERROR: Invalid line in file: %s\n", fname);
    }
    g_strfreev (e);
  }
  g_input_stream_close (G_INPUT_STREAM (istream), NULL, &error);
  return ids;
}



static void
cowmail_encrypt (const guchar *secret,
                 const guchar *iv,
                 gsize         n,
                 guchar       *crypto,
                 const guchar *clear)
{
  /* derive message encryption key */
  guchar aeskey[COWMAIL_KEY_SIZE];
  gnutls_hash_fast (GNUTLS_DIG_SHA256, secret, CURVE25519_SIZE, aeskey);

  /* encrypt payload */
  struct gcm_aes256_ctx aes;
  gcm_aes256_set_key (&aes, aeskey);
  gcm_aes256_set_iv (&aes, COWMAIL_TAG_SIZE, iv);
  gcm_aes256_encrypt (&aes, n, crypto, clear);
  gcm_aes256_digest (&aes, COWMAIL_TAG_SIZE, crypto + n);

  /* delete secrets */
  memset (&aes, 0, sizeof (struct gcm_aes256_ctx));
  memset (aeskey, 0, COWMAIL_KEY_SIZE);
}



static gboolean
cowmail_decrypt (const guchar *secret,
                 const guchar *iv,
                 gsize         n,
                 guchar       *clear,
                 const guchar *crypto)
{
  /* derive message encryption key */
  guchar aeskey[COWMAIL_KEY_SIZE];
  gnutls_hash_fast (GNUTLS_DIG_SHA256, secret, CURVE25519_SIZE, aeskey);

  /* decrypt payload */
  struct gcm_aes256_ctx aes;
  gcm_aes256_set_key (&aes, aeskey);
  gcm_aes256_set_iv (&aes, COWMAIL_TAG_SIZE, iv);
  gcm_aes256_decrypt (&aes, n, clear, crypto);
  guchar tag[COWMAIL_TAG_SIZE];
  gcm_aes256_digest (&aes, COWMAIL_TAG_SIZE, tag);

  /* delete secrets */
  memset (&aes, 0, sizeof (struct gcm_aes256_ctx));
  memset (aeskey, 0, COWMAIL_KEY_SIZE);

  /* check GCM authentication tag */
  return (memcmp (tag, crypto + n, COWMAIL_TAG_SIZE) == 0);
}



static guchar *
cowmail_encrypt_msg (const cowmail_id *id,
                     const gchar      *msg,
                     gsize             n)
{
  guchar *result = g_malloc0 (COWMAIL_HEAD_SIZE + COWMAIL_TAG_SIZE + n);
  guchar *pkey = result;
  guchar *chash = result + COWMAIL_KEY_SIZE;
  guchar *cmsg = result + COWMAIL_HEAD_SIZE;

  /* generate new ElGamal keypair, store pubkey in output */
  guchar skey[CURVE25519_SIZE];
  gnutls_rnd (GNUTLS_RND_KEY, skey, CURVE25519_SIZE);
  curve25519_mul_g (pkey, skey);

  /* compute master secret */
  guchar secret[CURVE25519_SIZE];
  curve25519_mul (secret, skey, id->pkey);

  /* encrypt payload */
  cowmail_encrypt (secret, pkey + COWMAIL_TAG_SIZE, n, cmsg, (guchar *) msg);

  /* compute message hash */
  guchar hash[COWMAIL_KEY_SIZE];
  gnutls_hash_fast (GNUTLS_DIG_SHA256, cmsg, n + COWMAIL_TAG_SIZE, hash);

  /* encrypt message hash */
  cowmail_encrypt (secret, pkey, COWMAIL_KEY_SIZE, chash, hash);

  /* clear secrets from memory */
  memset (skey, 0, CURVE25519_SIZE);

  return result;
}



static cowmail_ticket *
cowmail_decrypt_head (const cowmail_id *id,
                      const guchar     *head)
{
  const guchar *pkey = head;
  const guchar *chash = head + COWMAIL_KEY_SIZE;

  cowmail_ticket *ticket = g_malloc (sizeof (cowmail_ticket));

  /* compute master secret */
  guchar secret[CURVE25519_SIZE];
  curve25519_mul (secret, id->skey, pkey);

  if (cowmail_decrypt (secret, pkey, COWMAIL_KEY_SIZE, ticket->hash, chash)) {
    memcpy (ticket->secret, secret, COWMAIL_KEY_SIZE);
    memcpy (ticket->nonce, pkey + COWMAIL_TAG_SIZE, COWMAIL_TAG_SIZE);
    return ticket;
  }
  g_free (ticket);
  return NULL;
}



static gchar *
cowmail_decrypt_msg (cowmail_ticket   *ticket,
                     const guchar     *cmsg,
                     gsize             len)
{
  if (len < COWMAIL_TAG_SIZE)
    return NULL;

  gsize n = len - COWMAIL_TAG_SIZE;
  guchar *msg = g_malloc (n);

   if (cowmail_decrypt (ticket->secret, ticket->nonce, n, msg, cmsg))
    return (gchar *) msg;

  g_printerr ("COWMAIL ERROR: Auth tag missmatch.\n");
  g_free (msg);
  return NULL;
}



void
cowmail_put (const gchar      *hostname,
             const gchar      *msg,
             const cowmail_id *id)
{
  g_autoptr (GError) error = NULL;
  gsize n = strlen (msg) + 1;
  gsize len = COWMAIL_HEAD_SIZE + COWMAIL_TAG_SIZE + n;
  g_autofree guchar *cryptotext = cowmail_encrypt_msg (id, msg, n);

  g_autoptr (GSocketClient) client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  g_autoptr (GSocketConnection) connection;
  if ((connection = g_socket_client_connect_to_host (client, hostname, COWMAIL_DEFAULT_PORT, NULL, &error))) {
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    g_output_stream_write (ostream, cryptotext, len, NULL, &error);
    g_io_stream_close (G_IO_STREAM (connection), NULL, &error);
  } else {
    g_printerr ("COWMAIL ERROR PUT: %s\n", error->message);
  }
}



GList *
cowmail_list (const gchar      *hostname,
              const cowmail_id *id)
{
  g_autoptr (GError) error = NULL;
  GList *hashes = NULL;

  g_autoptr (GSocketClient) client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  g_autoptr (GSocketConnection) connection = g_socket_client_connect_to_host (client, hostname, COWMAIL_DEFAULT_PORT, NULL, &error);
  if (!error) {
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    g_output_stream_write (ostream, "", 1, NULL, &error);

    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
    guchar head[COWMAIL_HEAD_SIZE];
    while (g_input_stream_read (istream, head, COWMAIL_HEAD_SIZE, NULL, &error) == COWMAIL_HEAD_SIZE) {
      cowmail_ticket *t = cowmail_decrypt_head (id, head);
      if (t)
        hashes = g_list_prepend (hashes, t);
    }
    g_io_stream_close (G_IO_STREAM (connection), NULL, NULL);
  } else {
    g_printerr ("COWMAIL ERROR LIST: %s\n", error->message);
  }
  return hashes;
}



gchar *
cowmail_get (const gchar      *hostname,
             cowmail_ticket   *ticket)
{
  g_autoptr (GError) error = NULL;
  g_autoptr (GSocketClient) client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);

  gchar *message = NULL;
  GSocketConnection *connection = g_socket_client_connect_to_host (client, hostname, COWMAIL_DEFAULT_PORT, NULL, &error);
  if (!error) {
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    GInputStream *istream = g_io_stream_get_input_stream (G_IO_STREAM (connection));
    g_output_stream_write (ostream, ticket->hash, COWMAIL_KEY_SIZE, NULL, &error);
    gsize len;
    guchar response[65536];
    if ((len = g_input_stream_read (istream, response, 65536, NULL, &error)) > COWMAIL_TAG_SIZE) {
      message = cowmail_decrypt_msg (ticket, response, len);
    }
    g_io_stream_close (G_IO_STREAM (connection), NULL, &error);
  } else {
    g_printerr ("COWMAIL ERROR GET: %s\n", error->message);
  }
  return (gchar *) message;
}



void
cowmail_crypto_test (cowmail_id *id)
{
  guchar skey[CURVE25519_SIZE];
  gnutls_rnd (GNUTLS_RND_KEY, skey, CURVE25519_SIZE);
  guchar pkey[CURVE25519_SIZE];
  curve25519_mul_g (pkey, skey);

  gchar msg[] = "This is a encryption test without network.";
  gsize n = strlen (msg) + 1;
  g_print ("CRYPTO TEST: Encrypting ... [%s]\n", msg);

  /* compute master secret */
  guchar secret[CURVE25519_SIZE];
  curve25519_mul (secret, skey, id->pkey);

  guchar cmsg[n + COWMAIL_TAG_SIZE];
  cowmail_encrypt (secret, pkey, n, cmsg, (guchar *) msg);

  guchar decmsg[n];
  if (cowmail_decrypt (secret, pkey, n, decmsg, cmsg))
    g_print ("CRYPTO TEST: Decrypted ... [%s]\n", (gchar *) decmsg);
  else
    g_print ("CRYPTO TEST: Auth tag missmatch.\n");
}



void
cowmail_protocol_test (const gchar *server,
                       cowmail_id  *id)
{
  gchar msg[] = "This is a network protocol test.";
  g_print ("COWMAIL TEST: Sending PUT command. Message: [%s]\n", msg);
  cowmail_put (server, msg, id);
  g_print ("COWMAIL TEST: Sending LIST command.\n");
  GList *hashes = cowmail_list (server, id);

  g_print ("COWMAIL TEST: Sending GET command. Hashes: [%p]\n", hashes);

  for (GList *h = hashes; h; h = h->next) {
    gchar *backmsg = cowmail_get (server, h->data);
    if (backmsg)
      g_print ("COWMAIL TEST: Message received: [%s].\n", backmsg);
    else
      g_print ("COWMAIL TEST: No message received.\n");
  }
}
