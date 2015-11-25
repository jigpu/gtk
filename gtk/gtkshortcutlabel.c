/* gtkshortcutlabel.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gtkshortcutlabelprivate.h"
#include "gtklabel.h"
#include "gtkframe.h"
#include "gtkstylecontext.h"
#include "gtkprivate.h"
#include "gtkintl.h"

struct _GtkShortcutLabel
{
  GtkBox  parent_instance;
  gchar  *accelerator;
};

struct _GtkShortcutLabelClass
{
  GtkBoxClass parent_class;
};

G_DEFINE_TYPE (GtkShortcutLabel, gtk_shortcut_label, GTK_TYPE_BOX)

enum {
  PROP_0,
  PROP_ACCELERATOR,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static gchar **
get_labels (guint key, GdkModifierType modifier, guint *n_mods)
{
  const gchar *labels[16];
  gchar key_label[6];
  gchar *tmp;
  gunichar ch;
  gint i = 0;

  if (modifier & GDK_SHIFT_MASK)
    labels[i++] = C_("keyboard label", "Shift");
  if (modifier & GDK_CONTROL_MASK)
    labels[i++] = C_("keyboard label", "Ctrl");
  if (modifier & GDK_MOD1_MASK)
    labels[i++] = C_("keyboard label", "Alt");
  if (modifier & GDK_MOD2_MASK)
    labels[i++] = "Mod2";
  if (modifier & GDK_MOD3_MASK)
    labels[i++] = "Mod3";
  if (modifier & GDK_MOD4_MASK)
    labels[i++] = "Mod4";
  if (modifier & GDK_MOD5_MASK)
    labels[i++] = "Mod5";
  if (modifier & GDK_SUPER_MASK)
    labels[i++] = C_("keyboard label", "Super");
  if (modifier & GDK_HYPER_MASK)
    labels[i++] = C_("keyboard label", "Hyper");
  if (modifier & GDK_META_MASK)
    labels[i++] = C_("keyboard label", "Meta");

  *n_mods = i;

  ch = gdk_keyval_to_unicode (key);
  if (ch && ch < 0x80 && g_unichar_isgraph (ch))
    {
      switch (ch)
        {
        case '\\':
          labels[i++] = C_("keyboard label", "Backslash");
          break;
        default:
          memset (key_label, 0, 6);
          g_unichar_to_utf8 (g_unichar_toupper (ch), key_label);
          labels[i++] = key_label;
          break;
        }
    }
  else
    {
      switch (key)
        {
        case GDK_KEY_Left:
          labels[i++] = "\xe2\x86\x90";
          break;
        case GDK_KEY_Up:
          labels[i++] = "\xe2\x86\x91";
          break;
        case GDK_KEY_Right:
          labels[i++] = "\xe2\x86\x92";
          break;
        case GDK_KEY_Down:
          labels[i++] = "\xe2\x86\x93";
          break;
        case GDK_KEY_space:
          labels[i++] = "\xe2\x90\xa3";
          break;
        case GDK_KEY_Return:
          labels[i++] = "\xe2\x8f\x8e";
          break;
        case GDK_KEY_Page_Up:
          labels[i++] = C_("keyboard label", "Page_Up");
          break;
        case GDK_KEY_Page_Down:
          labels[i++] = C_("keyboard label", "Page_Down");
          break;
        default:
          tmp = gdk_keyval_name (gdk_keyval_to_lower (key));
          if (tmp != NULL)
            {
              if (tmp[0] != 0 && tmp[1] == 0)
                {
                  key_label[0] = g_ascii_toupper (tmp[0]);
                  key_label[1] = '\0';
                  labels[i++] = key_label;
                }
              else
                {
                  labels[i++] = g_dpgettext2 (GETTEXT_PACKAGE, "keyboard label", tmp);
                }
            }
        }
    }

  labels[i] = NULL;

  return g_strdupv ((gchar **)labels);
}

static GtkWidget *
dim_label (const gchar *text)
{
  GtkWidget *label;

  label = gtk_label_new (text);
  gtk_widget_show (label);
  gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");

  return label;
}

static void
display_shortcut (GtkContainer    *self,
                  guint            key,
                  GdkModifierType  modifier)
{
  gchar **keys = NULL;
  gint i;
  guint n_mods;

  keys = get_labels (key, modifier, &n_mods);
  for (i = 0; keys[i]; i++)
    {
      GtkWidget *frame;
      GtkWidget *disp;

      if (i > 0)
        gtk_container_add (self, dim_label ("+"));

      frame = gtk_frame_new (NULL);
      gtk_style_context_add_class (gtk_widget_get_style_context (frame), "keycap");
      gtk_widget_show (frame);

      gtk_container_add (self, frame);

      if (i < n_mods)
        gtk_widget_set_size_request (frame, 50, -1);

      disp = gtk_label_new (keys[i]);
      gtk_widget_show (disp);
      gtk_container_add (GTK_CONTAINER (frame), disp);
    }
  g_strfreev (keys);
}

static gboolean
parse_sequence (GtkShortcutLabel *self,
                const gchar      *str)
{
  gchar **accels;
  gint k;
  GdkModifierType modifier = 0;
  guint key = 0;
  gboolean retval = TRUE;

  accels = g_strsplit (str, "+", 0);
  for (k = 0; accels[k]; k++)
    {
      gtk_accelerator_parse (accels[k], &key, &modifier);
      if (key == 0 && modifier == 0)
        {
          retval = FALSE;
          break;
        }
      display_shortcut (GTK_CONTAINER (self), key, modifier);
    }
  g_strfreev (accels);

  return retval;
}

static gboolean
parse_range (GtkShortcutLabel *self,
             const gchar      *str)
{
  gchar *dots;

  dots = strstr (str, "...");
  if (!dots)
    return parse_sequence (self, str);

  dots[0] = '\0';
  if (!parse_sequence (self, str))
    return FALSE;

  gtk_container_add (GTK_CONTAINER (self), dim_label ("⋯"));
  if (!parse_sequence (self, dots + 3))
    return FALSE;

  return TRUE;
}

static void
gtk_shortcut_label_rebuild (GtkShortcutLabel *self)
{
  gchar **accels;
  gint k;

  gtk_container_foreach (GTK_CONTAINER (self), (GtkCallback)gtk_widget_destroy, NULL);

  if (self->accelerator == NULL)
    return;

  accels = g_strsplit (self->accelerator, " ", 0);
  for (k = 0; accels[k]; k++)
    {
      if (k > 0)
        gtk_container_add (GTK_CONTAINER (self), dim_label ("/"));

      if (!parse_range (self, accels[k]))
        {
          g_warning ("Failed to parse %s, part of accelerator '%s'", accels[k], self->accelerator);
          break;
        }
    }
  g_strfreev (accels);
}

static void
gtk_shortcut_label_finalize (GObject *object)
{
  GtkShortcutLabel *self = (GtkShortcutLabel *)object;

  g_free (self->accelerator);

  G_OBJECT_CLASS (gtk_shortcut_label_parent_class)->finalize (object);
}

static void
gtk_shortcut_label_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GtkShortcutLabel *self = GTK_SHORTCUT_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      g_value_set_string (value, gtk_shortcut_label_get_accelerator (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_shortcut_label_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GtkShortcutLabel *self = GTK_SHORTCUT_LABEL (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      gtk_shortcut_label_set_accelerator (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_shortcut_label_class_init (GtkShortcutLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gtk_shortcut_label_finalize;
  object_class->get_property = gtk_shortcut_label_get_property;
  object_class->set_property = gtk_shortcut_label_set_property;

  properties[PROP_ACCELERATOR] =
    g_param_spec_string ("accelerator", P_("Accelerator"), P_("Accelerator"),
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
gtk_shortcut_label_init (GtkShortcutLabel *self)
{
  gtk_box_set_spacing (GTK_BOX (self), 6);
}

GtkWidget *
gtk_shortcut_label_new (const gchar *accelerator)
{
  return g_object_new (GTK_TYPE_SHORTCUT_LABEL,
                       "accelerator", accelerator,
                       NULL);
}

const gchar *
gtk_shortcut_label_get_accelerator (GtkShortcutLabel *self)
{
  g_return_val_if_fail (GTK_IS_SHORTCUT_LABEL (self), NULL);

  return self->accelerator;
}

void
gtk_shortcut_label_set_accelerator (GtkShortcutLabel *self,
                                    const gchar      *accelerator)
{
  g_return_if_fail (GTK_IS_SHORTCUT_LABEL (self));

  if (g_strcmp0 (accelerator, self->accelerator) != 0)
    {
      g_free (self->accelerator);
      self->accelerator = g_strdup (accelerator);
      gtk_shortcut_label_rebuild (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCELERATOR]);
    }
}
