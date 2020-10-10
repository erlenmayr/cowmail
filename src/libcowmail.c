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



identity *
identity_new (gchar  *name,
              guchar *pkey,
              guchar *skey)
{
  identity *id = g_malloc (sizeof (identity));
  id->name = name;
  id->pkey = pkey;
  id-> skey = skey;
  return id;
}



identity *
identity_generate (gchar  *name)
{
  identity *id = g_malloc (sizeof (identity));
  unsigned char *pkey = g_malloc (crypto_box_PUBLICKEYBYTES);
  unsigned char *skey = g_malloc (crypto_box_SECRETKEYBYTES);
  crypto_box_keypair (pkey, skey);
  id->name = name;
  id->pkey = pkey;
  id-> skey = skey;
  return id;
}



void
identity_free (void *p)
{
  identity *id = p;
  if (id->name)
    g_free (id->name);
  if (id->pkey)
    g_free (id->pkey);
  if (id->skey)
    g_free (id-> skey);
  g_free (id);
}



head *
head_new (gchar         *hash,
          identity      *id)
{
  head *h = g_malloc (sizeof (head));
  h->hash = hash;
  h->id = id;
  return h;
}



void
head_free (void *p)
{
  head *h = p;
  if (h->hash)
    g_free (h->hash);
  //if (h->id)
  //  identity_free (h->id);
  g_free(h);
}



void
put_message (const gchar *hostname,
             guint16      port,
             const gchar *msg,
             const gchar *b64pkey)
{
  gsize pklen;
  guchar *pkey = g_base64_decode (b64pkey, &pklen);
  guchar cryptotext[crypto_box_SEALBYTES + strlen (msg) + 1];
  crypto_box_seal (cryptotext, (guchar *) msg, strlen (msg) + 1, pkey);
  gchar *b64msg = g_base64_encode (cryptotext, sizeof (cryptotext));

  gchar *clearhash = g_compute_checksum_for_string (G_CHECKSUM_SHA256, b64msg, -1);
  guchar cryptohash[crypto_box_SEALBYTES + strlen (clearhash) + 1];
  crypto_box_seal (cryptohash, (guchar *) clearhash, strlen (clearhash) + 1, pkey);
  gchar *b64hash = g_base64_encode (cryptohash, sizeof (cryptohash));

  GSocketClient *client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  GSocketConnection *connection = g_socket_client_connect_to_host (client, hostname, port, NULL, NULL);
  GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));

  gchar *payload = g_strconcat ("PUT ", b64hash, " ", b64msg, "\n", NULL);
  g_output_stream_write (ostream, payload, strlen (payload), NULL, NULL);

  g_io_stream_close (G_IO_STREAM (connection), NULL, NULL);
  g_free (payload);
  g_object_unref (connection);
  g_object_unref (client);
  g_free (b64msg);
  g_free (pkey);
}



GSList *
list_heads (const gchar *hostname,
           guint16       port,
           GSList       *identities)
{
  g_autoptr (GError) error = NULL;
  GSList *hashes = NULL;
  GSocketClient *client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  GSocketConnection *connection = g_socket_client_connect_to_host (client, hostname, port, NULL, &error);
  if (!error) {
    GDataOutputStream *ostream = g_data_output_stream_new (g_io_stream_get_output_stream (G_IO_STREAM (connection)));
    GDataInputStream *dstream = g_data_input_stream_new (g_io_stream_get_input_stream (G_IO_STREAM (connection)));
    g_data_output_stream_put_string (ostream, "LIST\n", NULL, NULL);
    gchar *msg;
    while ((msg = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, NULL))) {
      gsize len;
      gchar *cryptotext = (gchar *) g_base64_decode (msg, &len);
      for (GSList *id = identities; id; id = id->next) {
        guchar *hash = g_malloc (len - crypto_box_SEALBYTES);
        if (!crypto_box_seal_open (hash, (guchar *) cryptotext,
                                   len,
                                   ((identity *) id->data)->pkey,
                                   ((identity *) id->data)->skey)) {
          head *h = head_new ((gchar *) hash, (identity *) id->data);
          hashes = g_slist_prepend (hashes, h);
        } else {
          g_printerr ("Could not decrypt hash with identity %s.\n", ((identity *) id->data)->name);
        }
      }
      free (msg);
    }
    g_object_unref (dstream);
    g_object_unref (ostream);
    g_io_stream_close (G_IO_STREAM (connection), NULL, NULL);
    g_object_unref (connection);
  } else {
    g_printerr ("list_heads(): %s\n", error->message);
  }
  g_object_unref (client);
  return hashes;
}



gchar *
get_message (const gchar  *hostname,
             guint16       port,
             head         *h)
{
  g_autoptr (GError) error = NULL;
  guchar *message = NULL;
  GSocketClient *client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  GSocketConnection *connection = g_socket_client_connect_to_host (client, hostname, port, NULL, &error);
  if (!error) {
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    GDataInputStream *dstream = g_data_input_stream_new (g_io_stream_get_input_stream (G_IO_STREAM (connection)));
    gchar *command = g_strconcat ("GET ", h->hash, "\n", NULL);
    g_output_stream_write (ostream, command, strlen (command), NULL, NULL);
    g_free (command);
    gchar *response = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, &error);
    if (!error) {
      gsize len;
      gchar *cryptotext = (gchar *) g_base64_decode (response, &len);
      message = g_malloc (len - crypto_box_SEALBYTES);
      g_free (response);
      if (crypto_box_seal_open (message, (guchar *) cryptotext, len, h->id->pkey, h->id->skey)) {
        g_printerr ("Decryption of message from %s failed.\n", hostname);
        g_free(message);
        message = NULL;
      }
    } else {
      g_printerr ("%s\n", error->message);
      g_error_free (error);
    }
    g_io_stream_close (G_IO_STREAM (connection), NULL, &error);
    g_object_unref (dstream);
    g_object_unref (connection);
  } else {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
  }
  g_object_unref (client);
  return (gchar *) message;
}
