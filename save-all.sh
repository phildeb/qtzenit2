#!/bin/sh
echo "-- starting backup  --"
date
TODAY=`date +%Y%m%d`
echo $TODAY
TGZTARGET=../all-zenitel-debreuil-$TODAY.tgz
tar cvfz $TGZTARGET /vega/ /etc/inittab /etc/crontab /etc/fstab
mysqldump -uzenitel -pzenitel zenitel > zenitel-$TODAY.sql
echo "-- backup done  --"
ls -ltr $TGZTARGET

