#!/bin/sh -e

which gnome-autogen.sh || {
    echo "gnome-autogen.sh not found, you need to install gnome-common"
    exit 1
}

. gnome-autogen.sh
