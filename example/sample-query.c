/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2016 Iain Lane <iain@orangesquash.org.uk>
 *
 * Licensed under the GNU Lesser General Public License Version 3
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <libmo/mo.h>

#include <glib/gprintf.h>

#include <stdlib.h>

static gchar *
read_one_translation (MoFile *mofile, GError **err)
{
        if (!mofile) {
                g_printerr ("couldn't get translation: %s\n", (*err)->message);
                g_clear_error (err);
                return NULL;
        }

        return mo_file_get_translation (mofile,
                                        "edit the source information file",
                                        err);
}

static int
read_all_translations (void)
{
        GError *err = NULL;
        g_autoptr(GList) domains_head = NULL;
        GList * domains;

        g_autoptr(MoGroup) mogroup = mo_group_new ("apt", &err);

        if (!mogroup) {
                g_printerr ("couldn't create MoGroup: %s\n", err->message);
                g_clear_error (&err);

                return EXIT_FAILURE;
        }

        domains = domains_head = mo_group_get_languages (mogroup);

        while (domains) {
                g_autofree gchar *translation;
                g_autoptr(MoFile) mofile = mo_group_get_mo_file (mogroup,
                                                                 (gchar *) domains->data);
                translation = read_one_translation (mofile, &err);

                if (translation) {
                        g_printf ("%s: '%s'\n",
                                  (gchar *) domains->data,
                                  translation);
                } else {
                        g_printf ("%s: no translation found (%s)\n", (gchar *) domains->data, err->message);
                        g_clear_error (&err);
                }

                domains = domains->next;
        }

        return EXIT_SUCCESS;
}

static int
read_all_translations2 (void)
{
        GError *err = NULL;
        g_autoptr(GHashTable) ht = NULL;
        GHashTableIter iter;
        void *key, *value;

        g_autoptr(MoGroup) mogroup = mo_group_new ("apt", &err);

        if (!mogroup) {
                g_printerr ("couldn't create MoGroup: %s\n", err->message);
                g_clear_error (&err);

                return EXIT_FAILURE;
        }

        ht = mo_group_get_translations (mogroup,
                                        "edit the source information file");

        g_hash_table_iter_init (&iter, ht);

        while (g_hash_table_iter_next (&iter, &key, &value))
                g_printf ("%s: '%s'\n", (gchar *) key, (gchar *) value);

        return EXIT_SUCCESS;
}

int
main (int argc     G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
        g_autofree gchar *one = NULL;
        g_autofree gchar *contents = NULL;
        gsize length;
        int all, all2;

        GError *err = NULL;

        g_autoptr(MoFile) mofile = mo_file_new ("/usr/share/locale/de/LC_MESSAGES/apt.mo", &err);
        g_autoptr(MoFile) mofile2 = NULL;

        g_print ("Reading just the one translation...\n");
        one = read_one_translation (mofile, &err);
        if (one)
                g_printf ("%s\n", one);

        g_print ("Reading all translations, by iterating the domains...\n");
        all = read_all_translations ();

        g_print ("Reading all translations directly...\n");
        all2 = read_all_translations2 ();

        g_print ("Reading the file in first...\n");
        g_file_get_contents("/usr/share/locale/de/LC_MESSAGES/apt.mo", &contents, &length, &err);

        mofile2 = mo_file_new_from_bytes (g_bytes_new(contents, length), &err);

        one = read_one_translation (mofile2, &err);
        if (err)
                g_printf("omg %s\n", err->message);
        if (one)
                g_printf ("%s\n", one);

        if (!one || all == EXIT_FAILURE || all2 == EXIT_FAILURE)
                return EXIT_FAILURE;

        return EXIT_SUCCESS;
}
