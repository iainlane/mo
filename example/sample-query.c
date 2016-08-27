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

#include <mo.h>

#include <glib/gprintf.h>

int
main (int argc     G_GNUC_UNUSED,
      char *argv[] G_GNUC_UNUSED)
{
        MoFile *mofile = mo_file_new ("/usr/share/locale-langpack/fr/LC_MESSAGES/apt.mo");

        g_autofree gchar *trans = mo_file_get_translation (mofile,
                                                           "edit the source information file");

        g_printf ("%s\n", trans);

        g_object_unref (mofile);
}