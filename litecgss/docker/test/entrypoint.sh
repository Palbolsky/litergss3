#!/bin/sh -e

cmake -DCMAKE_BUILD_TYPE=Debug .

echo "Project configured, build is now starting"

cmake --build .

for executable in $(find ./bin/ -perm -700 -type f -name 'LiteCGSS_*_test'); do
	echo "Executing $executable..."
	$executable
done

cd /html
gcovr -r /litecgss --exclude=/litecgss/external --exclude=/litecgss/test --html --html-details -o coverage.html