#!/usr/bin/env bash
#########################################################################
# Author: blaketang
# Created Time: Wed Jan  3 23:08:32 2024
# File Name: get-from-exported-har.sh
# Description: 
#########################################################################

#cat ~/Downloads/aaa.har| jq -r '.log.entries[0].request.postData.text' >image2-curl-post-body.bin
#cat ~/Downloads/aaa.har| jq -r '.log.entries[0].request.postData.mimeType' >image2-curl-content-type.txt

FILE=two-text-file.har
FILE=curl-one-png-file.har
cat ~/Downloads/$FILE| jq -r '.log.entries[0].request.postData.text' >curl-body-$FILE.bin
cat ~/Downloads/$FILE| jq -r '.log.entries[0].request.postData.mimeType' >curl-content-type-$FILE.txt
