#!/bin/bash
export LANG=C
export LC_CTYPE=C
export LC_ALL=C

cd $(dirname "$0")


cp src/include/* ./dist/include/zbase/

cp README.md ./dist/include/zbase/
cp LICENSE ./dist/include/zbase/

last_sha1=`git rev-parse HEAD`
last_date=`git show --pretty=format:"%ci" | head -1`
echo "version:" > ./dist/include/zbase/VERSION
echo "last_sha1=$last_sha1" >> ./dist/include/zbase/VERSION 
echo "last_date=$last_date" >> ./dist/include/zbase/VERSION 
echo "" >> ./dist/include/zbase/VERSION 
echo "git log:" >> ./dist/include/zbase/VERSION 
git log -1 --stat ./src >> ./dist/include/zbase/VERSION 
