#!/bin/sh
echo "starting vega control" 
strace   /usr/vega/bin/vegactrl -r /usr/vega/share/ -w /usr/vega/conf/
