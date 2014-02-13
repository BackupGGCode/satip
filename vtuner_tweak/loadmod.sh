rmmod vtunerc
cp vtunerc.ko /lib/modules/`uname -r`/kernel/drivers/media/tuners/vtunerc.ko 
depmod -a
modprobe vtunerc
sleep 1
chmod a+rw /dev/vtunerc0
chmod a+rw /dev/dvb/adapter0/*
