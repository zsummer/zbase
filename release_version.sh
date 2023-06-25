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

last_dist_sha1=`git log -1 --stat ./src |grep -E "commit ([0-9a-f]*)" |grep -E -o "[0-9a-f]{10,}"`
last_dist_date=`git show $last_dist_sha1 --pretty=format:"%ci" | head -1`
echo "zbase last commit:"
echo $last_sha1
echo $last_date
git log -1 --stat ./src


echo "./src last commit:"
echo $last_dist_sha1
echo $last_dist_date
git log -1 --stat ./src

echo "write versions"
echo "version:" > ./dist/include/zbase/VERSION
echo "last_sha1(./src)=$last_dist_sha1" >> ./dist/include/zbase/VERSION 
echo "last_date(./src)=$last_dist_date" >> ./dist/include/zbase/VERSION 
echo "" >> ./dist/include/zbase/VERSION 
echo "git log -1 --stat ./src:" >> ./dist/include/zbase/VERSION 
echo "write done"




