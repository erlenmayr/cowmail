/* cowmail-window.c
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

#include "cowmail-config.h"
#include "cowmail-window.h"
#include "write-window.h"


typedef struct
{
  gchar  *name;
  guchar *pkey;
  guchar *skey;
} identity;



static identity *
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



static void
identity_free (void *p)
{
  identity *id = p;
  g_free (id->name);
  g_free (id->pkey);
  g_free (id-> skey);
  g_free (id);
}



struct _CowmailWindow
{
  GtkApplicationWindow  parent_instance;

  GtkHeaderBar         *header_bar;
  GtkButton            *bn_new;
  GtkButton            *bn_update;
  GtkEntry             *en_hostname;
  GtkButton            *mn_add_id;

  GtkListStore         *ls_messages;
  GtkListStore         *ls_contacts;
  GtkListStore         *ls_identities;

  GtkTreeView          *tv_contacts;
  GtkCellRenderer      *rd_ct_name;
  GtkTreeView          *tv_identities;
  GtkCellRenderer      *rd_id_name;
};

G_DEFINE_TYPE (CowmailWindow, cowmail_window, GTK_TYPE_APPLICATION_WINDOW)



static GSList *
get_identities (CowmailWindow *self)
{
  GSList *list = NULL;
  GtkTreeModel *l = GTK_TREE_MODEL (self->ls_identities);
  GtkTreeIter i;
  if (gtk_tree_model_get_iter_first (l, &i)) {
    do {
      gsize len;
      GValue value = G_VALUE_INIT;
      gtk_tree_model_get_value (l, &i, 0, &value);
      gchar *name = g_value_dup_string (&value);
      g_value_unset (&value);
      gtk_tree_model_get_value (l, &i, 1, &value);
      gchar *b64pkey = g_value_dup_string (&value);
      guchar *pkey = g_base64_decode (b64pkey, &len);
      g_free (b64pkey);
      g_value_unset (&value);
      gtk_tree_model_get_value (l, &i, 2, &value);
      gchar *b64skey = g_value_dup_string (&value);
      guchar *skey = g_base64_decode (b64skey, &len);
      g_free (b64skey);
      g_value_unset (&value);

      identity *id = identity_new (name, pkey, skey);
      list = g_slist_prepend (list, id);
    } while (gtk_tree_model_iter_next (l, &i));
  }
  return list;
}



static GSList *
get_contacts (CowmailWindow *self)
{
  GSList *list = NULL;
  GtkTreeModel *l = GTK_TREE_MODEL (self->ls_contacts);
  GtkTreeIter i;
  if (gtk_tree_model_get_iter_first (l, &i)) {
    do {
      gsize len;
      GValue value = G_VALUE_INIT;
      gtk_tree_model_get_value (l, &i, 0, &value);
      gchar *name = g_value_dup_string (&value);
      g_value_unset (&value);
      gtk_tree_model_get_value (l, &i, 1, &value);
      gchar *b64pkey = g_value_dup_string (&value);
      guchar *pkey = g_base64_decode (b64pkey, &len);
      g_free (b64pkey);
      g_value_unset (&value);

      identity *id = identity_new (name, pkey, NULL);
      list = g_slist_prepend (list, id);
    } while (gtk_tree_model_iter_next (l, &i));
  }
  return list;
}



static gint
load_identities (CowmailWindow *self)
{
  gint count = 0;
  gchar *idpath = g_strjoin("/", g_get_user_config_dir (), "cowmail/identities.conf", NULL);
  GFile *file = g_file_new_for_path(idpath);
  GFileInputStream *istream = g_file_read(file, NULL, NULL);
  if (istream) {
    GDataInputStream *dstream = g_data_input_stream_new (G_INPUT_STREAM (istream));
    gchar *line;
    while ((line = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, NULL))) {
      gchar **e = g_strsplit_set (line, " \n", 4);
      GtkTreeIter iter;
      gtk_list_store_insert_with_values (self->ls_identities, &iter, 0,
                                         0, e[0],
                                         1, e[1],
                                         2, e[2],
                                         -1);
      g_strfreev (e);
      count++;
    }
    g_object_unref (dstream);
    g_input_stream_close (G_INPUT_STREAM (istream), NULL, NULL);
  }
  g_object_unref (file);
  g_free (idpath);
  return count;
}



static void
store_identities (CowmailWindow *self)
{
  gchar *confpath = g_strjoin("/", g_get_user_config_dir (), "cowmail", NULL);
  GFile *confdir = g_file_new_for_path(confpath);
  g_file_make_directory (confdir, NULL, NULL);
  gchar *idpath = g_strjoin("/", g_get_user_config_dir (), "cowmail/identities.conf", NULL);
  GFile *file = g_file_new_for_path(idpath);
  GFileIOStream *iostream = g_file_replace_readwrite (file, NULL, TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL);
  GDataOutputStream *dstream = g_data_output_stream_new (g_io_stream_get_output_stream (G_IO_STREAM (iostream)));
  GSList *ids = get_identities (self);
  for (GSList *id = ids; id; id = id->next) {
    gchar *b64pkey = g_base64_encode (((identity *) id->data)->pkey, crypto_box_PUBLICKEYBYTES);
    gchar *b64skey = g_base64_encode (((identity *) id->data)->skey, crypto_box_SECRETKEYBYTES);
    gchar *line = g_strjoin (" ", ((identity *) id->data)->name, b64pkey, b64skey, "\n", NULL);
    g_data_output_stream_put_string (dstream, line, NULL, NULL);
    g_free (line);
    g_free (b64skey);
    g_free (b64pkey);
  }
  g_io_stream_close (G_IO_STREAM (iostream), NULL, NULL);
}



static void
load_contacts (CowmailWindow *self)
{
  gchar *idpath = g_strjoin("/", g_get_user_config_dir (), "cowmail/contacts.conf", NULL);
  GFile *file = g_file_new_for_path(idpath);
  GFileInputStream *istream = g_file_read(file, NULL, NULL);
  if (istream) {
    GDataInputStream *dstream = g_data_input_stream_new (G_INPUT_STREAM (istream));
    gchar *line;
    while ((line = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, NULL))) {
      gchar **e = g_strsplit_set (line, " \n", 4);
      GtkTreeIter iter;
      gtk_list_store_insert_with_values (self->ls_contacts, &iter, 0,
                                         0, e[0],
                                         1, e[1],
                                         -1);
      g_strfreev (e);
    }
    g_object_unref (dstream);
    g_input_stream_close (G_INPUT_STREAM (istream), NULL, NULL);
  }
  g_object_unref (file);
  g_free (idpath);
}



static void
store_contacts (CowmailWindow *self)
{
  gchar *confpath = g_strjoin("/", g_get_user_config_dir (), "cowmail", NULL);
  GFile *confdir = g_file_new_for_path(confpath);
  g_file_make_directory (confdir, NULL, NULL);
  gchar *idpath = g_strjoin("/", g_get_user_config_dir (), "cowmail/contacts.conf", NULL);
  GFile *file = g_file_new_for_path(idpath);
  GFileIOStream *iostream = g_file_replace_readwrite (file, NULL, TRUE, G_FILE_CREATE_REPLACE_DESTINATION, NULL, NULL);
  GDataOutputStream *dstream = g_data_output_stream_new (g_io_stream_get_output_stream (G_IO_STREAM (iostream)));
  GSList *ids = get_contacts (self);
  for (GSList *id = ids; id; id = id->next) {
    gchar *b64pkey = g_base64_encode (((identity *) id->data)->pkey, crypto_box_PUBLICKEYBYTES);
    gchar *line = g_strjoin (" ", ((identity *) id->data)->name, b64pkey, "\n", NULL);
    g_data_output_stream_put_string (dstream, line, NULL, NULL);
    g_free (line);
    g_free (b64pkey);
  }
  g_io_stream_close (G_IO_STREAM (iostream), NULL, NULL);
}



static void
get_message (const gchar  *hostname,
             identity     *id,
             gchar        *hash,
             GtkListStore *ls_messages)
{
  GSocketClient *client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  GSocketConnection *connection;
  if ((connection = g_socket_client_connect_to_host (client, hostname, 1337, NULL, NULL))) {
    GOutputStream *ostream = g_io_stream_get_output_stream (G_IO_STREAM (connection));
    GDataInputStream *dstream = g_data_input_stream_new (g_io_stream_get_input_stream (G_IO_STREAM (connection)));
    gchar *command = g_strconcat ("GET ", hash, "\n", NULL);
    g_output_stream_write (ostream, command, strlen (command), NULL, NULL);
    g_free (command);
    gchar *msg;
    while ((msg = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, NULL))) {
      gsize len;
      gchar *cryptotext = (gchar *) g_base64_decode (msg, &len);
      guchar message[len];
      if (crypto_box_seal_open (message, (guchar *) cryptotext, len, id->pkey, id->skey)) {
        GtkTreeIter iter;
        gtk_list_store_insert_with_values (ls_messages, &iter, 0,
                                           0, id->name,
                                           1, "[DECRYPTION FAILED]",
                                           2, g_get_real_time (),
                                           -1);
      } else {
        GtkTreeIter iter;
        gtk_list_store_insert_with_values (ls_messages, &iter, 0,
                                           0, id->name,
                                           1, (gchar *) message,
                                           2, g_get_real_time (),
                                           -1);
      }
      free (msg);
    }
    g_io_stream_close (G_IO_STREAM (connection), NULL, NULL);
    g_object_unref (connection);
  }
  g_object_unref (client);
}



static void
get_hash_list (const gchar  *hostname,
               GSList       *identities,
               GtkListStore *ls_messages)
{
  GSocketClient *client = g_socket_client_new ();
  g_socket_client_set_protocol (client, G_SOCKET_PROTOCOL_SCTP);
  GSocketConnection *connection;
  if ((connection = g_socket_client_connect_to_host (client, hostname, 1337, NULL, NULL))) {
    GDataOutputStream *ostream = g_data_output_stream_new (g_io_stream_get_output_stream (G_IO_STREAM (connection)));
    GDataInputStream *dstream = g_data_input_stream_new (g_io_stream_get_input_stream (G_IO_STREAM (connection)));
    g_data_output_stream_put_string (ostream, "LIST\n", NULL, NULL);
    gchar *msg;
    while ((msg = g_data_input_stream_read_line_utf8 (dstream, NULL, NULL, NULL))) {
      gsize len;
      g_print ("Trying to open: [%s]\n", msg);
      gchar *cryptotext = (gchar *) g_base64_decode (msg, &len);
      guchar message[len];
      for (GSList *id = identities; id; id = id->next) {
        if (!crypto_box_seal_open (message, (guchar *) cryptotext,
                                   len,
                                   ((identity *) id->data)->pkey,
                                   ((identity *) id->data)->skey)) {
          g_print("Opening succeeded.\n");
          get_message(hostname, (identity *) id->data, (gchar *) message, ls_messages);
        } else {
          g_print("Opening failed.\n");
        }
      }
      free (msg);
    }
    g_object_unref (dstream);
    g_object_unref (ostream);
    g_io_stream_close (G_IO_STREAM (connection), NULL, NULL);
    g_object_unref (connection);
  }
  g_object_unref (client);
}



static void
add_identity (CowmailWindow *self)
{
  unsigned char *pkey = g_malloc (crypto_box_PUBLICKEYBYTES);
  unsigned char *skey = g_malloc (crypto_box_SECRETKEYBYTES);
  crypto_box_keypair (pkey, skey);

  gchar *b64pkey = (gchar *) g_base64_encode (pkey, crypto_box_PUBLICKEYBYTES);
  gchar *b64skey = (gchar *) g_base64_encode (skey, crypto_box_SECRETKEYBYTES);
  gchar *address = g_strndup (b64pkey, 8);

  GtkTreeIter iter;
  gtk_list_store_insert_with_values (self->ls_identities, &iter, 0,
                                     0, address,
                                     1, b64pkey,
                                     2, b64skey,
                                     -1);
  g_free (b64skey);
  g_free (b64pkey);
}



static void
on_bn_new_clicked (GtkButton *button,
                   gpointer   userdata)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (userdata);
  CowmailWindow *self = COWMAIL_WINDOW (userdata);

  WriteWindow *win = g_object_new (WRITE_TYPE_WINDOW,
		                               "application", gtk_window_get_application (GTK_WINDOW (self)),
		                               NULL);
  write_window_set_addresses (win, self->ls_contacts);
  write_window_set_hostname (win, gtk_entry_get_text (self->en_hostname), 1337);
  gtk_window_present (GTK_WINDOW (win));
}



static void
on_bn_update_clicked (GtkButton *button,
                      gpointer   userdata)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (userdata);
  CowmailWindow *self = COWMAIL_WINDOW (userdata);

  GSList *identities = get_identities (self);
  get_hash_list(gtk_entry_get_text (self->en_hostname),
                                    identities,
                                    self->ls_messages);
  g_slist_free_full (identities, identity_free);
}



static void
on_mn_add_id_clicked (GtkButton *button,
                      gpointer   userdata)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (userdata);
  CowmailWindow *self = COWMAIL_WINDOW (userdata);
  add_identity (self);
}



static void
on_mn_new_contact_clicked (GtkButton *button,
                           gpointer   userdata)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (userdata);
  CowmailWindow *self = COWMAIL_WINDOW (userdata);
  GtkTreeIter iter;
  gtk_list_store_insert (self->ls_contacts, &iter, 0);
}



static void
on_CowmailWindow_destroy (CowmailWindow *win,
                         gpointer      userdata)
{
  COWMAIL_IS_WINDOW (win);
  COWMAIL_IS_WINDOW (userdata);
  CowmailWindow *self = COWMAIL_WINDOW (win);
  store_identities (self);
  store_contacts (self);
  g_print ("Stored identities and contacts.\n");
}



static void
cb_name_edited (GtkCellRenderer *cell,
                gchar           *path,
                gchar           *new,
                gpointer         userdata)
{
  GTK_IS_CELL_RENDERER (cell);
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeModel *treemodel = gtk_tree_view_get_model (treeview);
  GtkListStore *liststore = GTK_LIST_STORE (treemodel);
  GtkTreeIter iter;
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (liststore), &iter, path);
  gtk_list_store_set (liststore, &iter, 0, new, -1);
}



static void
cb_pkey_edited (GtkCellRenderer *cell,
                gchar           *path,
                gchar           *new,
                gpointer         userdata)
{
  GTK_IS_CELL_RENDERER (cell);
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeModel *treemodel = gtk_tree_view_get_model (treeview);
  GtkListStore *liststore = GTK_LIST_STORE (treemodel);
  GtkTreeIter iter;
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (liststore), &iter, path);
  gtk_list_store_set (liststore, &iter,
                      1, new,
                      -1);
/*
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (GTK_TREE_MODEL (liststore), &iter, 0, &value);
  gchar *name = g_value_dup_string (&value);
  g_value_unset (&value);
  if (g_str_equal (name, "")) {
    gchar *address = g_strndup(new, 8);
    gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (liststore), &iter, path);
    gtk_list_store_set (liststore, &iter,
                        0, address,
                        -1);
    g_free (address);
  }
  g_free (name);
  */
}



static void
cowmail_window_class_init (CowmailWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ch/verbuecheln/cowmail/cowmail-window.ui");

  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, bn_new);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, bn_update);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, en_hostname);

  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, mn_add_id);

  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, ls_messages);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, ls_contacts);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, ls_identities);

  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, tv_contacts);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, rd_ct_name);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, tv_identities);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, rd_id_name);

  gtk_widget_class_bind_template_callback (widget_class, on_bn_new_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_update_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_mn_add_id_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_mn_new_contact_clicked);
  gtk_widget_class_bind_template_callback (widget_class, cb_name_edited);
  gtk_widget_class_bind_template_callback (widget_class, cb_pkey_edited);
  gtk_widget_class_bind_template_callback (widget_class, on_CowmailWindow_destroy);
}



static void
cowmail_window_init (CowmailWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  if (load_identities (self) == 0) {
    add_identity (self);
  }
  load_contacts (self);
}