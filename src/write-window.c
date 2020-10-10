/* write-window.c
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

#include "write-window.h"

struct _WriteWindow
{
  GtkWindow      parent_instance;
  GtkHeaderBar  *header_bar;
  GtkComboBox   *cb_recipient;
  GtkButton     *bn_send;

  GtkTextBuffer *tb_message;

  const gchar   *hostname;
  guint16        port;
};

G_DEFINE_TYPE (WriteWindow, write_window, GTK_TYPE_WINDOW)



void
write_window_set_addresses (WriteWindow  *self,
                            GtkListStore *recipients)
{
  gtk_combo_box_set_model (self->cb_recipient, GTK_TREE_MODEL (recipients));
  gtk_combo_box_set_id_column (self->cb_recipient, 1);
  gtk_combo_box_set_active (self->cb_recipient, 0);
}



void
write_window_set_hostname (WriteWindow *self,
                           const gchar *hostname,
                           guint16      port)
{
  self->hostname = hostname;
  self->port = port;
}



static void
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



static void
on_bn_send_clicked (GtkButton *button,
                    gpointer   userdata)
{
  GTK_IS_BUTTON (button);
  WRITE_IS_WINDOW (userdata);
  WriteWindow *win = WRITE_WINDOW (userdata);

  GtkTreeIter iter;
  gtk_combo_box_get_active_iter (win->cb_recipient, &iter);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (gtk_combo_box_get_model (win->cb_recipient), &iter, 1, &value);
  gchar *b64pkey = g_value_dup_string (&value);
  g_value_unset (&value);

  g_print ("Encrypting with: [%s]\n", b64pkey);

  GtkTextIter siter, eiter;
  gtk_text_buffer_get_start_iter (win->tb_message, &siter);
  gtk_text_buffer_get_end_iter (win->tb_message, &eiter);
  gchar *msg = gtk_text_buffer_get_text (win->tb_message, &siter, &eiter, FALSE);

  put_message (win->hostname, win->port, msg, b64pkey);

  g_free (msg);
  g_free (b64pkey);
  gtk_window_close (GTK_WINDOW (win));
}



static void
write_window_class_init (WriteWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ch/verbuecheln/cowmail/write-window.ui");
  gtk_widget_class_bind_template_child (widget_class, WriteWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, WriteWindow, bn_send);
  gtk_widget_class_bind_template_child (widget_class, WriteWindow, cb_recipient);
  gtk_widget_class_bind_template_child (widget_class, WriteWindow, tb_message);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_send_clicked);
}



static void
write_window_init (WriteWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
