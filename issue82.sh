#!/bin/sh

USER_1=root
DIR=/usr/local/app/tars/tarsnode/data/XCommon.ActivityServer/bin
SOURCE=./ActivityServer
HOST_1=10.10.10.82

rsync -vz $SOURCE $USER_1@$HOST_1:$DIR
