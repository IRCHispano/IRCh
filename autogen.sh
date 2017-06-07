#!/bin/sh

aclocal && \
rm -rf autom4te.cache/ && \
autoheader && \
automake --foreign --add-missing &&
autoconf
