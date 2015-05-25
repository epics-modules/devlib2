#!/bin/sh
set -e

TAG=$1

exec git archive --prefix=devlib2-${1}/ --format tar.gz -9 -o devlib2-${1}.tar.gz ${1}
