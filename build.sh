#!/bin/sh

../generator/generator `pwd` underscore.lua

rm -fr output install
mkdir output
../luabuild/luabuild `pwd` `pwd`/output us-libs.lua  -- destdir=`pwd`/install

# This wont work yet
# ../luabuild/luabuild `pwd` `pwd`/output us-apps.lua  -- destdir=`pwd`/install

# Try a test or two first
gcc -g -ggdb2 -o net1.test -Iinstall/usr/include test/net/net1.c -Itest/lib/include test/lib/test.c -std=c99 -L`pwd`/install/lib -lus_network -lus_common -lus_thread -lus_job_queue -lus_encryption -lus_event -lz -pthread -D_GNU_SOURCE
gcc -g -ggdb2 -o net2.test -Iinstall/usr/include test/net/net2.c -Itest/lib/include test/lib/test.c -std=c99 -L`pwd`/install/lib -lus_network -lus_common -lus_thread -lus_job_queue -lus_encryption -lus_event -lz -pthread -D_GNU_SOURCE
