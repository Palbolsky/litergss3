#!/bin/sh -e

mkdir -p /tmp/build && cd /tmp/build

cmake -DCMAKE_BUILD_TYPE=Debug /litecgss
cmake --build /litecgss

for executable in $(find /litecgss/bin/ -perm -700 -type f -name 'LiteCGSS_*_test'); do
	echo "Executing $executable..."
	$executable
done

cd /html
gcovr -r /litecgss --exclude=/litecgss/external --exclude=/litecgss/test --html --html-details -o coverage.html