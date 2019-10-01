#!/bin/sh
echo "starting vega control" 
#ls -l /usr/vega/share/
valgrind --tool=massif --alloc-fn=g_mem_chunk_alloc   /usr/vega/bin/vegactrl -r /usr/vega/share/ -w /usr/vega/conf/
