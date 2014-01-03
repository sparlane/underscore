#!/bin/sh

rm -fr output install && mkdir output && ../luabuild/luabuild `pwd` `pwd`/output us-libs.lua  -- destdir=`pwd`/install && ../luabuild/luabuild `pwd` `pwd`/output us-apps.lua  -- destdir=`pwd`/install
