<h1>SAT>IP</h1>

SAT>IP client for Linux attaching
to vtuner driver of
https://github.com/BackupGGCode/vtuner

To be used e.g. with tvheadend, vdr, ...

A vtuner driver with some required tweaks is included in the repository.

First integration with tvheadend and XBMC successful.

## Releases: ##

2014-03-22  -  <a href='https://github.com/BackupGGCode/satip/raw/wiki/satip-0.0.2.tar.gz'>satip-0.0.2.tar.gz</a>
  * Fix: Keep satip configuration after TEARDOWN. VDR likely queries tuning status from driver and does not re-tune after restart
  * Fix: PIDs are no longer swallowed when PID scanning and channel switching occurs in parallel


2014-03-10  -  <a href='https://github.com/BackupGGCode/satip/raw/wiki/satip-0.0.1.tar.gz'>satip-0.0.1.tar.gz</a>
  * This is a first release for packaging with openelec.

## Todo: ##

  * ~~Teardown on voltage off~~  - done
  * teardown when frontend is closed (?)
  * RTP reception queuing
  * flexible FE settings?
  * real satip tuner status feedback
  * ...

## SAT>IP Specification: ##

http://www.satip.info/resources

## Overview: ##

<img src='https://web.archive.org/web/20150429175749/http://wiki.satip.googlecode.com/git/satipvtuner.png'>
