/* cowmail-write-window.c
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

#include "cowmail-write-window.h"

struct _CowmailWriteWindow
{
  GtkWindow      parent_instance;
  GtkHeaderBar  *header_bar;
  GtkTextBuffer *tb_message;

  const gchar   *hostname;

  GtkComboBox   *cb_contacts;
  GtkListStore  *ls_contacts;
};

G_DEFINE_TYPE (CowmailWriteWindow, cowmail_write_window, GTK_TYPE_WINDOW)



CowmailWriteWindow *
cowmail_write_window_new (const gchar *hostname,
                  GList       *contacts)
{
  CowmailWriteWindow *self = g_object_new (COWMAIL_TYPE_WRITE_WINDOW, NULL);
  self->hostname = hostname;
  for (GList *c = contacts; c; c = c->next) {
    cowmail_id *id = c->data;
    gtk_list_store_insert_with_values (self->ls_contacts, NULL, 0,
                                       0, id->name,
                                       1, id,
                                       -1);
  }
  gtk_combo_box_set_active (self->cb_contacts, 0);
  return self;
}



static void
on_bn_send_clicked (GtkButton          *button,
                    CowmailWriteWindow *self)
{
  GTK_IS_BUTTON (button);
  COWMAIL_IS_WRITE_WINDOW (self);

  GtkTextIter siter, eiter;
  gtk_text_buffer_get_start_iter (self->tb_message, &siter);
  gtk_text_buffer_get_end_iter (self->tb_message, &eiter);
  g_autofree gchar *msg = gtk_text_buffer_get_text (self->tb_message, &siter, &eiter, FALSE);

  GtkTreeIter iter;
  gtk_combo_box_get_active_iter (self->cb_contacts, &iter);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (gtk_combo_box_get_model (self->cb_contacts), &iter, 1, &value);

  cowmail_id *id = (cowmail_id *) g_value_get_pointer (&value);
  cowmail_put (self->hostname, msg, id);
  g_value_unset (&value);

  gtk_window_close (GTK_WINDOW (self));
}



static void
cowmail_write_window_class_init (CowmailWriteWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/ch/verbuecheln/cowmail/cowmail-write-window.ui");
  gtk_widget_class_bind_template_child (widget_class, CowmailWriteWindow, header_bar);
  gtk_widget_class_bind_template_child (widget_class, CowmailWriteWindow, tb_message);
  gtk_widget_class_bind_template_child (widget_class, CowmailWriteWindow, cb_contacts);
  gtk_widget_class_bind_template_child (widget_class, CowmailWriteWindow, ls_contacts);
  gtk_widget_class_bind_template_callback (widget_class, on_bn_send_clicked);
}



static void
cowmail_write_window_init (CowmailWriteWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
