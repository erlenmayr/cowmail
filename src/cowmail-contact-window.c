/* cowmail-contact-window.c
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

#include "cowmail-contact-window.h"

struct _CowmailContactWindow
{
  GtkWindow      parent_instance;
  GtkHeaderBar  *header_bar;
  GtkButton     *bn_save;

  GtkListBox    *lb_contacts;
  GtkEntry      *en_name;
  GtkEntry      *en_pkey;
  GtkLabel      *la_warning;

  GList        **contacts;
};

G_DEFINE_TYPE (CowmailContactWindow, cowmail_contact_window, GTK_TYPE_WINDOW)



CowmailContactWindow *
cowmail_contact_window_new (GList **contacts)
{
  CowmailContactWindow *self = g_object_new (COWMAIL_TYPE_CONTACT_WINDOW, NULL);
  for (GList *c = *contacts; c; c = c->next) {
    gtk_list_box_prepend (self->lb_contacts, GTK_WIDGET (cowmail_contact_row_new (c->data)));
  }
  gtk_widget_show_all (GTK_WIDGET (self->lb_contacts));
  self->contacts = contacts;

  return self;
}



static void
on_bn_save_clicked (GtkButton            *self,
                    CowmailContactWindow *window)
{
  GTK_IS_BUTTON (self);
  COWMAIL_IS_CONTACT_WINDOW (window);

  GList *contacts = gtk_container_get_children (GTK_CONTAINER (window->lb_contacts));
  for (GList *c = contacts; c; c = c->next) {
    CowmailContactRow *row = c->data;
    cowmail_id *id = cowmail_id_from_key (cowmail_contact_row_get_name (row),
                                          cowmail_contact_row_get_pkey (row),
                                          NULL);
    c->data = id;
  }

  g_list_free (*(window->contacts));
  *(window->contacts) = contacts;

  g_autofree gchar *ctpath = g_strjoin ("/", g_get_user_config_dir (), "cowmail", "contacts.conf", NULL);
  g_autoptr (GFile) ctfile = g_file_new_for_path (ctpath);
  cowmail_ids_store (ctfile, contacts);

  gtk_widget_destroy (GTK_WIDGET (window));
}



static void
on_bn_new_clicked (GtkButton            *self,
                   CowmailContactWindow *window)
{
  GTK_IS_BUTTON (self);
  COWMAIL_IS_CONTACT_WINDOW (window);

  CowmailContactRow *row = cowmail_contact_row_new (NULL);
  gtk_list_box_prepend (window->lb_contacts, GTK_WIDGET (row));
  gtk_widget_show_all (GTK_WIDGET (row));
}



static void
on_lb_contacts_row_activated (GtkListBox           *self,
                              CowmailContactRow    *row,
                              CowmailContactWindow *window)
{
  GTK_IS_LIST_BOX (self);
  COWMAIL_IS_CONTACT_ROW (row);
  COWMAIL_IS_CONTACT_WINDOW (window);

  gtk_entry_set_text (window->en_name, cowmail_contact_row_get_name (row));
  g_autofree gchar *b64pkey = g_base64_encode (cowmail_contact_row_get_pkey (row), COWMAIL_KEY_SIZE);
  gtk_entry_set_text (window->en_pkey, b64pkey);
}



static void
on_en_name_changed (GtkEntry             *self,
                    CowmailContactWindow *window)
{
  GTK_IS_ENTRY (self);
  COWMAIL_IS_CONTACT_WINDOW (window);

  GtkListBoxRow *row = gtk_list_box_get_selected_row (window->lb_contacts);
  cowmail_contact_row_set_name (COWMAIL_CONTACT_ROW (row), gtk_entry_get_text (self));
}



static void
on_en_pkey_changed (GtkEntry             *self,
                    CowmailContactWindow *window)
{
  GTK_IS_ENTRY (self);
  COWMAIL_IS_CONTACT_WINDOW (window);

  gsize plen;
  g_autofree guchar *pkey = g_base64_decode (gtk_entry_get_text (self), &plen);

  if (plen == COWMAIL_KEY_SIZE) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row (window->lb_contacts);
    cowmail_contact_row_set_pkey (COWMAIL_CONTACT_ROW (row), pkey);
    gtk_label_set_text (window->la_warning, "");
  } else {
    gtk_label_set_text (window->la_warning, "WARNING: Not a valid Cowmail key.");
  }
}



static void
cowmail_contact_window_class_init (CowmailContactWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ch/verbuecheln/cowmail/cowmail-contact-window.ui");

  gtk_widget_class_bind_template_child (widget_class, CowmailContactWindow, bn_save);
  gtk_widget_class_bind_template_child (widget_class, CowmailContactWindow, lb_contacts);
  gtk_widget_class_bind_template_child (widget_class, CowmailContactWindow, en_name);
  gtk_widget_class_bind_template_child (widget_class, CowmailContactWindow, en_pkey);
  gtk_widget_class_bind_template_child (widget_class, CowmailContactWindow, la_warning);

  gtk_widget_class_bind_template_callback (widget_class, on_bn_save_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_new_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_lb_contacts_row_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_en_name_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_en_pkey_changed);
}



static void
cowmail_contact_window_init (CowmailContactWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
