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



struct _CowmailWindow
{
  GtkApplicationWindow  parent_instance;

  GtkEntry             *en_server;
  GtkAboutDialog       *dg_about;
  GtkListBox           *lb_messages;

  cowmail_id           *id;
  GList                *contacts;
};

G_DEFINE_TYPE (CowmailWindow, cowmail_window, GTK_TYPE_APPLICATION_WINDOW)



static void
on_bn_new_clicked (GtkButton     *button,
                   CowmailWindow *self)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (self);

  CowmailWriteWindow *win = cowmail_write_window_new (gtk_entry_get_text (self->en_server),
                                                      self->contacts);
  gtk_window_present (GTK_WINDOW (win));
}



static void
on_bn_update_clicked (GtkButton     *button,
                      CowmailWindow *self)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (self);

  GList *heads = cowmail_list (gtk_entry_get_text (self->en_server), self->id);
  for (GList *h = heads; h; h = h->next) {
    g_autofree gchar *msg = cowmail_get (gtk_entry_get_text (self->en_server), h->data);
    if (msg) {
      CowmailMsgRow *row = cowmail_msg_row_new (msg);
      gtk_list_box_prepend (self->lb_messages, GTK_WIDGET (row));
      gtk_widget_show_all (GTK_WIDGET (self->lb_messages));
    }
  }
  gtk_widget_show_all (GTK_WIDGET (self->lb_messages));
}



static void
on_bn_contacts_clicked (GtkButton     *button,
                    CowmailWindow *self)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (self);

  CowmailContactWindow *win = cowmail_contact_window_new (&(self->contacts));
  gtk_window_present (GTK_WINDOW (win));
}



static void
on_bn_test_clicked (GtkButton     *button,
                    CowmailWindow *self)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (self);

  cowmail_crypto_test (self->id);
  cowmail_protocol_test (gtk_entry_get_text (self->en_server));
}



static void
on_bn_about_clicked (GtkButton *button,
                     gpointer   userdata)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WINDOW (userdata);

  gtk_widget_show (GTK_WIDGET (COWMAIL_WINDOW (userdata)->dg_about));
}



static void
on_lb_messages_row_activated (GtkListBox    *self,
                              CowmailMsgRow *row,
                              GtkTextView   *display)
{
  GTK_IS_LIST_BOX (self);
  COWMAIL_IS_MSG_ROW (row);
  GTK_IS_TEXT_VIEW (display);

  GtkTextBuffer *buf = gtk_text_view_get_buffer (display);
  gtk_text_buffer_set_text (buf, cowmail_msg_row_get_msg (row), -1);
}



static void
cowmail_window_class_init (CowmailWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ch/verbuecheln/cowmail/cowmail-window.ui");

  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, en_server);
  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, dg_about);

  gtk_widget_class_bind_template_child (widget_class, CowmailWindow, lb_messages);

  gtk_widget_class_bind_template_callback (widget_class, on_bn_new_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_update_clicked);

  gtk_widget_class_bind_template_callback (widget_class, on_bn_contacts_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_test_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_about_clicked);

  gtk_widget_class_bind_template_callback (widget_class, gtk_widget_hide_on_delete);
  gtk_widget_class_bind_template_callback (widget_class, on_lb_messages_row_activated);
}



static void
cowmail_window_init (CowmailWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_about_dialog_set_version (self->dg_about, PACKAGE_VERSION);

  g_autofree gchar *confpath = g_strjoin ("/", g_get_user_config_dir (), "cowmail", NULL);
  g_autoptr (GFile) confdir = g_file_new_for_path (confpath);
  g_file_make_directory_with_parents (confdir, NULL, NULL);

  g_autofree gchar *idpath = g_strjoin ("/", g_get_user_config_dir (), "cowmail", "ids.conf", NULL);
  g_autoptr (GFile) idfile = g_file_new_for_path (idpath);
  GList *idlist = cowmail_ids_load (idfile);

  g_autofree gchar *ctpath = g_strjoin ("/", g_get_user_config_dir (), "cowmail", "contacts.conf", NULL);
  g_autoptr (GFile) ctfile = g_file_new_for_path (ctpath);
  GList *ctlist = cowmail_ids_load (ctfile);

  if (idlist) {
    self->id = idlist->data;
  } else {
    g_printerr ("COWMAIL INFO: Creating new ID.\n");
    self->id = cowmail_id_generate ("me");
    cowmail_id *contact = cowmail_id_to_contact (self->id);

    ctlist = g_list_prepend (ctlist, contact);
    idlist = g_list_append (NULL, self->id);
    cowmail_ids_store (idfile, idlist);
    cowmail_ids_store (ctfile, ctlist);
  }

  self->contacts = ctlist;
  g_list_free (idlist);
}
