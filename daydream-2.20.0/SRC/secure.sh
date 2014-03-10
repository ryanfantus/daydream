#!/bin/sh
DIR=$1

if [ -z "$DIR" ];
then
  echo "Run secure.sh /path/to/daydream"
  exit
fi

echo "Handling dirs..."
DIRS=`find $DIR -type d|xargs`
for i in $DIRS 
do
  chown bbs:bbs $i
  chmod 775 $i
done
echo "Handling files..."
FILES=`find $DIR/bulletins $DIR/configs/ $DIR/confs/ $DIR/data $DIR/display $DIR/fido $DIR/logfiles $DIR/questionnaire $DIR/temp $DIR/users -type f`
for i in $FILES
do
  chown bbs:bbs $i
  chmod 664 $i
done 
echo "Handling exec files..."
EFILES=`find $DIR/bin $DIR/doors $DIR/scripts $DIR/utils $DIR/batch -type f`
for i in $EFILES
do
  chown bbs:bbs $i
  chmod 775 $i
done 

chown zipcheck $DIR/utils/runas
chmod u+s $DIR/utils/runas
