#!/bin/sh
if [ `whoami` != "root" ];
then
  echo "Please run this script as root!"
  exit
fi

echo "Please enter the install path (e.g. /home/bbs)"
read INSTALL_PATH

echo "Please press control-c now if you haven't done a backup!!!"
read DAYDREAM_CONTINUE

cd SRC
sh make.sh build
INSTALL_PATH=$INSTALL_PATH sh make.sh install
cd ..

cd DOCS
if [ ! -d $INSTALL_PATH/docs ]
then
mkdir $INSTALL_PATH/docs
fi
cp -R * $INSTALL_PATH/docs
cd ..

echo "If you're upgrading from 2.14.9, you probably have the correct"
echo "inetd/xinetd settings already.  Howver, if you're upgrading"
echo "from 2.15 alpha, check out the for_inetd.conf or xinetd.howto"
echo "files so that you properly set ownerships and run the ftp"
echo "server."

echo ""
echo "Also, check out the new libdd.doc in the DOCS/ folder."
echo "It may be worthwhile to copy over your old libdd.doc."

echo ""
echo "One last thing - Check out the file UPGRADE for any last-"
echo "minute stuff to change."

echo ""
echo "Setup complete, restart inetd and telnet to localhost"
