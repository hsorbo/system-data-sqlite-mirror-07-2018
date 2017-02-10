#!/bin/bash

scriptdir=`dirname "$BASH_SOURCE"`
extradefs="$@"

if [[ "$OSTYPE" == "darwin"* ]]; then
  libname=libSQLite.Interop.dylib
  gccflags="-arch i386 -arch x86_64"
else
  libname=libSQLite.Interop.so
  gccflags=""
fi

pushd "$scriptdir/../SQLite.Interop/src/generic"
gcc -g -fPIC -shared $gccflags -o $libname interop.c -I../core -DSQLITE_THREADSAFE=1 -DSQLITE_USE_URI=1 -DSQLITE_ENABLE_COLUMN_METADATA=1 -DSQLITE_ENABLE_STAT4=1 -DSQLITE_ENABLE_FTS3=1 -DSQLITE_ENABLE_LOAD_EXTENSION=1 -DSQLITE_ENABLE_RTREE=1 -DSQLITE_SOUNDEX=1 -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1 -DSQLITE_ENABLE_API_ARMOR=1 -DSQLITE_ENABLE_DBSTAT_VTAB=1 -DINTEROP_TEST_EXTENSION=1 -DINTEROP_EXTENSION_FUNCTIONS=1 -DINTEROP_VIRTUAL_TABLE=1 -DINTEROP_FTS5_EXTENSION=1 -DINTEROP_PERCENTILE_EXTENSION=1 -DINTEROP_TOTYPE_EXTENSION=1 -DINTEROP_REGEXP_EXTENSION=1 -DINTEROP_JSON1_EXTENSION=1 -DINTEROP_SHA1_EXTENSION=1 $extradefs -lm -lpthread -ldl
mkdir -p ../../../bin/2013/Release/bin
mv $libname ../../../bin/2013/Release/bin
popd
