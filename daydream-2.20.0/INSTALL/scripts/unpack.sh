#!/bin/sh

PKTS=`find $INBOUND -iname *.mo? -or -iname *.tu? -or -iname *.we? -or -iname *.th? -or -iname *.fr? -or -iname *.sa? -or -iname *.su? | xargs`

for i in $PKTS 
do 
  x=`file -b $i`
  if [ -n "`echo $x|grep -i zip`" ];
  then
    unzip $i -d `dirname $i`
    [ -x /usr/bin/fromdos ] && fromdos in*/*.pkt || echo "Install package tofrodos for full functionality"
    if [ $? -eq 0 ]; then
      rm -f $i
    fi
  fi
done
