rmmod vtunerc
cp vtunerc.ko /lib/modules/`uname -r`/kernel/drivers/media/tuners/vtunerc.ko 
#cp vtunerc.ko /lib/modules/`uname -r`/kernel/drivers/media/dvb/vtunerc.ko
depmod -a
modprobe vtunerc devices=4 debug=0
sleep 1
chmod a+rw /dev/vtunerc*
chmod a+rw /dev/dvb/adapter0/*
chmod a+rw /dev/dvb/adapter1/*
chmod a+rw /dev/dvb/adapter2/*
chmod a+rw /dev/dvb/adapter3/*
