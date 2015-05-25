#!/bin/sh
set -e

die() {
    echo "$1"
    exit 1
}

[ "$USER" ] || die "USER not set"

[ -f documentation/mainpage.h ] || die "Run me in the top level"

(cd documentation && doxygen)

rsync -av --delete "$@" documentation/doc/html/ $USER,epics@frs.sourceforge.net:/home/project-web/epics/htdocs/devlib2/
