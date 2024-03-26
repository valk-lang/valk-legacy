#!/bin/sh

UNAME=$(uname)

if [ "$UNAME" = "Linux" ] ; then
	echo "# Linux install"
elif [ "$UNAME" = "Darwin" ] ; then
	echo "# macOS install"
else
	echo "Install script currently doesnt support: $UNAME"
	exit 1
fi

dir=$(dirname -- "$0";)
valk_dir="/opt/valk/__VERSION__"

sudo mkdir -p "$valk_dir"
sudo cp $dir/valk "$valk_dir/valk"
if [ ! -f "$valk_dir/lib" ]; then
	sudo rm -rf "$valk_dir/lib"
fi
sudo cp -r $dir/lib "$valk_dir/"
sudo mkdir -p /usr/local/bin
sudo ln -s -f "$valk_dir/valk" /usr/local/bin/vali

echo "# Done"
