#!/bin/sh
set -e

die() {
    echo "$1"
    exit 1
}

[ "$USER" ] || die "USER not set"

[ -f documentation/mainpage.h ] || die "Run me in the top level"

(cd documentation && doxygen)

git checkout gh-pages
cp -r documentation/doc/html/* .
rm -rf documentation
git add .
git commit -m "Last updates to documentation"
git push origin gh-pages
