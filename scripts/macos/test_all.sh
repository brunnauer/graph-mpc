#!/bin/bash

# AppleScript inside shell to open new Terminal windows and run commands
osascript <<EOF
tell application "Terminal"
    activate

    -- Window 1
    set w1 to do script "cd ~/CODE/GraphMPC/build/tests && ./test_sharing -p 0 --localhost --num_parties 2 --vec-size 100 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_shuffle -p 0 --localhost --vec-size 20 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_sort -p 0 --localhost --vec-size 100 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_sw_perm -p 0 --localhost --vec-size 10 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_mp -p 0 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_deduplication -p 0 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_pi_m -p 0 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_pi_k -p 0 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_bfs -p 0 --localhost"
    delay 0.2
    set bounds of front window to {0, 0, 500, 800} -- {left, top, right, bottom}

    -- Window 2
    set w2 to do script "cd ~/CODE/GraphMPC/build/tests && ./test_sharing -p 1 --localhost --num_parties 2 --vec-size 100 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_shuffle -p 1 --localhost --vec-size 20 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_sort -p 1 --localhost --vec-size 100 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_sw_perm -p 1 --localhost --vec-size 10 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_mp -p 1 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_deduplication -p 1 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_pi_m -p 1 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_pi_k -p 1 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_bfs -p 1 --localhost"
    delay 0.2
    set bounds of front window to {500, 0, 1000, 800}

    -- Window 3
    set w3 to do script "cd ~/CODE/GraphMPC/build/tests && ./test_shuffle -p 2 --localhost --vec-size 20 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_sort -p 2 --localhost --vec-size 100 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_sw_perm -p 2 --localhost --vec-size 10 \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_mp -p 2 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_deduplication -p 2 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_pi_m -p 2 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_pi_k -p 2 --localhost \
                      && cd ~/CODE/GraphMPC/build/tests && ./test_bfs -p 2 --localhost"
    delay 0.2
    set bounds of front window to {1000, 0, 1510, 800}
end tell
EOF