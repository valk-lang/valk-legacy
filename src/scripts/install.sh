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
volt_dir="/opt/volt/__VERSION__"

sudo mkdir -p "$volt_dir"
sudo cp $dir/volt "$volt_dir/volt"
if [ ! -f "$volt_dir/lib" ]; then
	sudo rm -rf "$volt_dir/lib"
fi
sudo cp -r $dir/lib "$volt_dir/"
sudo mkdir -p /usr/local/bin
sudo ln -s -f "$volt_dir/volt" /usr/local/bin/volt

echo "# Done"
