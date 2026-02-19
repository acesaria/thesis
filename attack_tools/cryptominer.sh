#!/bin/bash
while :; do echo $RANDOM | md5sum > /dev/null; done &
