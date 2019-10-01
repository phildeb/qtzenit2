
#!/bin/sh
echo "starting vega control" 
#ls -l /usr/vega/share/
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --error-limit=no --num-callers=50 /usr/vega/bin/vegactrl -r /usr/vega/share/ -w /usr/vega/conf/
