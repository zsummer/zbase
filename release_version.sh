#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

cd $(dirname "$0")


cp src/include/* ./dist/include/zbase/

cp README.md ./dist/include/zbase/
cp LICENSE ./dist/include/zbase/

version=`date +"release fn-log version date:%Y-%m-%d %H:%M:%S"`
echo $version > ./dist/include/zbase/VERSION 
echo "" >> ./dist/include/zbase/VERSION 
echo "git log:" >> ./dist/include/zbase/VERSION 
git log -1 --stat >> ./dist/include/zbase/VERSION 