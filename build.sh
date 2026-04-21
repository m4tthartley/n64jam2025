##
##	Created by Matt Hartley on 10/12/2025.
##	Copyright 2025 GiantJelly. All rights reserved.
##

# make
# mv ./n64jam2025.z64 ./build/


running=$(pgrep game)

set -e

if [ -z $running ]; then
	echo "Building executable..."
	clang sys_pc.c -g $(coreconfig) -lX11 -lGL -o ./build/game
else
	echo "Game is currently running"
fi

echo "Building game library..."
clang sys_pc.c -g $(coreconfig) -lX11 -lGL -o ./build/game.tmp.so -shared -fPIC
mv ./build/game.tmp.so ./build/game.so
