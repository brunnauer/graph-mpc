#!/bin/bash

cd ./build/tests/test_sharing && (./test_sharing -p 0 --localhost --num_parties 2 --vec-size 100 & ./test_sharing -p 1 --localhost --num_parties 2 --vec-size 100)
cd ../test_shuffle && (./test_shuffle -p 0 --localhost --vec-size 20 & ./test_shuffle -p 1 --localhost --vec-size 20 & ./test_shuffle -p 2 --localhost --vec-size 20)
cd ../test_sort && (./test_sort -p 0 --localhost --vec-size 100 & ./test_sort -p 1 --localhost --vec-size 100 & ./test_sort -p 2 --localhost --vec-size 100)
cd ../test_sw_perm && (./test_sw_perm -p 0 --localhost --vec-size 10 & ./test_sw_perm -p 1 --localhost --vec-size 10 & ./test_sw_perm -p 2 --localhost --vec-size 10)
cd ../test_mp && (./test_mp -p 0 --localhost & ./test_mp -p 1 --localhost & ./test_mp -p 2 --localhost)
cd ../test_bfs && (./test_bfs -p 0 --localhost & ./test_bfs -p 1 --localhost & ./test_bfs -p 2 --localhost)
