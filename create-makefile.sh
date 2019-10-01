#!/bin/sh
##
## create_makefile.sh
## 
## Made by mozveren
## Login   <mozveren@mon01>
## 
## Started on  Wed Mar 25 18:59:58 2009 mozveren
## Last update Wed Mar 25 18:59:58 2009 mozveren
##


#qmake -project
qmake -makefile
sed -i s/'DEFINES       ='/'DEFINES       = -DQT3_SUPPORT'/ Makefile
sed -i s/'LIBS          ='/'LIBS          = -lQtSql -lQtNetwork \.\/librudeconfig.a'/ Makefile
sed -i s@'INCPATH       ='@'INCPATH       = -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtSql'@ Makefile
