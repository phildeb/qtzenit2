#!/bin/sh
echo "starting vega control" 
gdb vegactrl -r /usr/vega/share/ -w /usr/vega/conf/
