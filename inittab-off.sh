#!/bin/sh
echo "###############" 
cat /etc/inittab | grep V1
echo "###############" 
sed -i s/'V1:'/'\#V1:'/ /etc/inittab
echo "###############" 
cat /etc/inittab | grep V1
echo "###############" 