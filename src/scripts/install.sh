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
vali_dir="/opt/vali/__VERSION__"

sudo mkdir -p "$vali_dir"
sudo cp $dir/vali "$vali_dir/vali"
if [ ! -f "$vali_dir/lib" ]; then
	sudo rm -rf "$vali_dir/lib"
fi
sudo cp -r $dir/lib "$vali_dir/"
sudo mkdir -p /usr/local/bin
sudo ln -s -f "$vali_dir/vali" /usr/local/bin/vali

echo "# Done"
