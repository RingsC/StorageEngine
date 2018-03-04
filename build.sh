#!/bin/bash
#build_mode='echo $1'
#make clean
#if [-z $build_mode ]
#then
#build_mode=debug
#fi
#if [ $build_mode != debug]
#echo $1
#. ./clean.sh

cd src
mv Makefile.global Makefile.global_copy
cp include/port/linux.h include/pg_config_os.h
cd ..

for option
do
    case "$option" in
        --enable-debug)  
            CPPFLAGS="$CPPFLAGS -g -D_DEBUG"
            CFLAGS="$CFLAGS -g -D_DEBUG"
            ;;
	--enable-pg)
	    CPPFLAGS="$CPPFLAGS -pg"
	    CFLAGS="$CFLAGS -pg"
	    LDFLAGS="$LDFLAGS -pg"
	    ;;   
        conf)
            ./configure --without-readline --without-zlib --disable-thread-safety;;
    *) echo $option;;
esac
done

cd src
rm -f Makefile.global
mv Makefile.global_copy Makefile.global

cd backend
make
err=$?

if [ $err -eq 2 ] ;
then
echo '11111'
exit -1
fi
cd ../../src/interface
make
err=$?
if [ $err -eq 2 ] ;
then
echo '222222'
exit -1
fi


cd ../../testStorageEngineDll
make
err=$?
if [ $err -eq 2 ] ;
then
echo '333333'
exit -1
fi


cd ../teststorageEngine
make
err=$?
if [ $err -eq 2 ] ;
then
echo '444444'
exit -1
fi

cd ../bootstrap
make
err=$?
if [ $err -eq 2 ] ;
then
echo '555555'
exit -1
fi

cd ../serverconfig
make
err=$?
if [ $err -eq 2 ] ;
then
exit -1
fi

cd ../walreceiver
make
err=$?
if [ $err -eq 2 ] ;
then
echo '6666666'
exit -1
fi

cd ../walsender
make
err=$?
if [ $err -eq 2 ] ;
then
exit -1
fi

cd ../baseBackup
make
err=$?
if [ $err -eq 2 ] ;
then
exit -1
fi

if test "$1" = "c" -o "$2" = "c" ;
then
cd ../
cp bin/initDataDir ../../../bin/linux/Debug/bootstrap_storage
cp bin/libpostgres.a ../../../bin/linux/Debug/libpostgres.a
cp bin/libstorageEngine.so ../../../bin/linux/Debug/libstorageEngine.so
cp bin/testEngineStorageDLL ../../../bin/linux/Debug/testStorageEngineDLL
cp bin/testEngineStorageLIB ../../../bin/linux/Debug/testStorageEngineLIB
fi

