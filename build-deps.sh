#!/bin/sh
set -e -x

# Build base for use with https://travis-ci.org
#
# Set environment variables
# BASE= 3.14 3.15 or 3.16  (VCS branch)
# STATIC=  static or shared

die() {
  echo "$1" >&2
  exit 1
}

[ "$BASE" ] || die "Set BASE"

CDIR="$HOME/.cache/base-$BASE-$STATIC"

if [ ! -e "$CDIR/built" ]
then
  install -d "$CDIR"
  ( cd "$CDIR" && git clone --depth 50 --branch $BASE https://github.com/epics-base/epics-base.git base )

  EPICS_BASE="$CDIR/base"

  case "$STATIC" in
  static)
    cat << EOF >> "$EPICS_BASE/configure/CONFIG_SITE"
SHARED_LIBRARIES=NO
STATIC_BUILD=YES
EOF
    ;;
  *) ;;
  esac

  make -C "$EPICS_BASE" -j2

  touch "$CDIR/built"
fi

EPICS_HOST_ARCH=`sh $EPICS_BASE/startup/EpicsHostArch`

echo "EPICS_BASE=$EPICS_BASE" > configure/RELEASE.local
