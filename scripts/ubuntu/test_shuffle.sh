#!/bin/bash

tmux new-session -d -s session1 'cd ./build/tests/test_shuffle && ./test_shuffle -p 0 --localhost'
tmux new-session -d -s session2 'cd ./build/tests/test_shuffle && ./test_shuffle -p 1 --localhost'
tmux new-session -d -s session3 'cd ./build/tests/test_shuffle && ./test_shuffle -p 2 --localhost'