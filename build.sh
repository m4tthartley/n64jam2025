##
##	Created by Matt Hartley on 10/12/2025.
##	Copyright 2025 GiantJelly. All rights reserved.
##

# make
# mv ./n64jam2025.z64 ./build/

# startTime=$(date +%s.%N)

running=$(pgrep game)

# echo "running = $running"

if [ -z $running ]; then
	echo "Building executable..."
	clang coretest.c -g $(coreconfig) -lX11 -lGL -o ./build/game
else
	echo "Game is currently running"
fi

echo "Building game library..."
clang coretest.c -g $(coreconfig) -lX11 -lGL -o ./build/game.tmp.so -shared -fPIC
mv ./build/game.tmp.so ./build/game.so
# ./build/game

# endTime=$(date +%s.%N)
# echo $(expr $endTime - $startTime)