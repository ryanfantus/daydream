cur_dir=`pwd`
targets="lib main ddz hydracom doors ddfv utils ddecho net ddtick"
if [ "x$MAKE" = "x" ] ; then
MAKE=make
fi

case "$1" in
  clean)
    for i in $targets
    do
      cd $cur_dir/$i
      $MAKE clean
    done
    cd $cur_dir/python
    $MAKE clean
    ;;
  build)
    
    for i in $targets
    do
      cd $cur_dir/$i
      $MAKE clean
      $MAKE
      if [ $? != 0 ]; 
      then
        echo "Build failed!"
        exit 1
      fi
      cd $cur_dir
    done

    if [ -z $PYTHON_H ];
    then
      ph=`find /usr/include/python2.7/ -name Python.h|head -n1`
      if [ -z $ph ];
      then
        ph=`find /usr/local/include/ -name Python.h|head -n1`
        if [ -z $ph ];
        then
          echo "Couldn't find your Python.h, use PYTHON_H=/path/to/Python.h sh make.sh build"
	  exit 1
        fi
      fi
      PYTHON_H=$ph
    fi

    phd=`dirname $PYTHON_H`
    cd $cur_dir/python
    $MAKE clean
    echo $phd
    $MAKE PYTHON_INCLUDE=$phd
    if [ $? != 0 ]; 
    then
      echo "Build failed!"
      exit 1
    fi
    cd $cur_dir
    echo "Build complete now run 'sudo INSTALL_PATH=/dir sh make.sh install'"

    ;;
  install)
    if [ -z $INSTALL_PATH ];
    then
      echo "Please run this script by: INSTALL_PATH=/dir sh make.sh install"
      exit
    fi

    for i in $targets
    do
      cd $cur_dir/$i
      $MAKE install INSTALL_PATH=$INSTALL_PATH
      cd $cur_dir
    done
    
    cd $cur_dir/python
    $MAKE install INSTALL_PATH=$INSTALL_PATH
    cd $cur_dir

    if [ ! -d $INSTALL_PATH/include ]
    then
    mkdir $INSTALL_PATH/include
    fi
    cp config.h $INSTALL_PATH/include
    cp main/*.h $INSTALL_PATH/include
    cp lib/*.h $INSTALL_PATH/include
    ;;
  
  *)
    echo "Run 'sh make.sh build' and after 'sh make.sh install INSTALL_PATH=/dir'"
    ;;
esac

