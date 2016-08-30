# What
This is a small GLib-based library to let you work directly with [`.mo`
files](https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html).

# Why
[GNU gettext](https://www.gnu.org/software/gettext/), and apparently everything
else, provide no functionality for dealing with `.mo` files without cumbersome,
slow and thread-unsafe procedures involving generating locales and setting
environment variables. Sometimes you have the file to hand and just want to get
at its contents without going through all that fuss.  This library will help
you do that.

[![Build Status](https://travis-ci.org/iainlane/mo.svg?branch=master)](https://travis-ci.org/iainlane/mo)
