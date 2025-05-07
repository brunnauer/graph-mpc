#!/bin/bash

# AppleScript inside shell to open new Terminal windows and run commands
osascript <<EOF
tell application "Terminal"
    activate

    -- Window 1
    set w1 to do script "cd ~/CODE/GraphMPC/build/tests/test_sharing && ./test_sharing -p 1 --localhost --num_parties 2"
    delay 0.2
    set bounds of front window to {500, 0, 1000, 800}

end tell
EOF