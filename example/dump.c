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

#include <locale.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
        char *filename;

        gchar *orig, *trans;
        g_autoptr(GHashTable) ht = NULL;
        GHashTableIter iter;
        g_autoptr(MoFile) mofile = NULL;

        GError *err = NULL;

        setlocale (LC_ALL, "");

        if (argc != 2) {
                g_printerr ("Usage: %s [filename]\n", argv[0]);
                return EXIT_FAILURE;
        }

        filename = argv[1];

        if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
                g_printerr ("Error: File '%s' does not exist\n", filename);
                return EXIT_FAILURE;
        }

        mofile = mo_file_new (filename, &err);

        if (!mofile) {
                g_printerr ("Error: File '%s' could not be read: %s\n", filename, err->message);
                g_error_free (err);
                return EXIT_FAILURE;
        }

        g_print ("Dumping...\n");

        ht = mo_file_get_translations (mofile);

        g_hash_table_iter_init (&iter, ht);

        while (g_hash_table_iter_next (&iter, (gpointer *) &orig, (gpointer *) &trans)) {
                g_print ("Orig: '%s'\nTranslation: '%s'\n\n", g_strstrip(orig), g_strstrip(trans));
        }

        return EXIT_SUCCESS;
}

