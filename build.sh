##
##	Created by Matt Hartley on 10/12/2025.
##	Copyright 2025 GiantJelly. All rights reserved.
##

# make
# mv ./n64jam2025.z64 ./build/

clang coretest.c -g $(coreconfig) -lX11 -lGL -o ./build/game
# ./build/game