#!/bin/sh
#
# Change FIDODIR=/home/bbs/fido to your actual echomail dir. Beneath that should be /in, /in.sec, /in.loc, /out
#
FIDODIR=/home/bbs/fido
PKTS=`find $INBOUND -iname *.mo? -or -iname *.tu? -or -iname *.we? -or -iname *.th? -or -iname *.fr? -or -iname *.sa? -or -iname *.su? | xargs`

for i in $PKTS 
do 
  x=`file -b $i`
  if [ -n "`echo $x|grep -i zip`" ];
  then
    unzip $i -d `dirname $i`
    [ -x /usr/bin/fromdos ] && fromdos $FIDODIR/in*/*.pkt || echo "Install package tofrodos for full functionality"
    if [ $? -eq 0 ]; then
      rm -f $i
    fi
  fi
done
