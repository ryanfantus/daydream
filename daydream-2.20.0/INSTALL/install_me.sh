#!/bin/sh

if [ -z "$1" ];
then
  echo "Run sh install_me.sh <path-to-install>"
fi

INSTALL_PATH=$1

if [ -n `echo $INSTALL_PATH|grep -e "/$"` ];
then
  INSTALL_PATH=`echo $INSTALL_PATH| sed "s/\/$//"`
fi

mkdir -p $INSTALL_PATH

cp -r batch/ $INSTALL_PATH

X="2 3 10 11 12 13 14"
for i in $X
do
  ln -f -s $INSTALL_PATH/batch/batch1.logoff $INSTALL_PATH/batch/batch$i.logoff
done

# bbs dirs
mkdir $INSTALL_PATH/bin
cp -r bulletins/ $INSTALL_PATH
mkdir $INSTALL_PATH/configs
mkdir $INSTALL_PATH/configs/ddtop_designs
mkdir $INSTALL_PATH/confs
cp -r data/ $INSTALL_PATH
cp -r display/ $INSTALL_PATH
mkdir $INSTALL_PATH/doors
mkdir $INSTALL_PATH/lib
mkdir $INSTALL_PATH/logfiles
cp -r python/ $INSTALL_PATH
cp -r questionnaire/ $INSTALL_PATH
cp -r scripts/ $INSTALL_PATH
mkdir $INSTALL_PATH/temp
mkdir $INSTALL_PATH/users
mkdir $INSTALL_PATH/utils
# fido dirs
mkdir $INSTALL_PATH/fido
mkdir $INSTALL_PATH/fido/in
mkdir $INSTALL_PATH/fido/in.sec
mkdir $INSTALL_PATH/fido/in.loc
mkdir $INSTALL_PATH/fido/out
mkdir $INSTALL_PATH/fido/bad
mkdir $INSTALL_PATH/fido/lists
mkdir $INSTALL_PATH/fido/announce

SED_EXPR_FROM=`echo "/home/bbs"|sed 's/\\//\\\\\\//g'`
SED_EXPR_TO=`echo "$INSTALL_PATH"|sed 's/\\//\\\\\\//g'`

for i in `find configs/ -type f|xargs`
do
  cat $i| sed --posix -e "s/$SED_EXPR_FROM/$SED_EXPR_TO/g" > $INSTALL_PATH/$i
done

echo "export LD_LIBRARY_PATH=$INSTALL_PATH/lib" > $INSTALL_PATH/scripts/ddenv.sh
echo "export PATH=$INSTALL_PATH/bin:$INSTALL_PATH/utils:\$PATH" >> $INSTALL_PATH/scripts/ddenv.sh
echo "export DDECHO=$INSTALL_PATH/configs/dd-echo.cfg" >> $INSTALL_PATH/scripts/ddenv.sh
echo "export DDTICK=$INSTALL_PATH/configs/dd-tick.cfg" >> $INSTALL_PATH/scripts/ddenv.sh
echo "export DAYDREAM=$INSTALL_PATH" >> $INSTALL_PATH/scripts/ddenv.sh

echo "#!/bin/sh" > $INSTALL_PATH/scripts/ddlogin.sh
echo ". $INSTALL_PATH/scripts/ddenv.sh" >> $INSTALL_PATH/scripts/ddlogin.sh
echo "exec $INSTALL_PATH/bin/daydream" >> $INSTALL_PATH/scripts/ddlogin.sh

chmod ugo+x $INSTALL_PATH/scripts/ddlogin.sh

echo "#!/bin/sh" > $INSTALL_PATH/scripts/runftp.sh
echo ". $INSTALL_PATH/scripts/ddenv.sh" >> $INSTALL_PATH/scripts/runftp.sh
echo "$INSTALL_PATH/bin/ddftpd ddftpd -D$INSTALL_PATH -p$INSTALL_PATH/bin/daydream" >> $INSTALL_PATH/scripts/runftp.sh

chmod ugo+x $INSTALL_PATH/scripts/runftp.sh

echo "telnet stream tcp nowait root /usr/sbin/tcpd $INSTALL_PATH/bin/ddtelnetd -u bbs" > $INSTALL_PATH/for_inetd.conf
echo "ftp stream tcp nowait root /usr/sbin/tcpd $INSTALL_PATH/scripts/runftp.sh" >> $INSTALL_PATH/for_inetd.conf

echo "Install successfull, now compile the source and install the binaries"
