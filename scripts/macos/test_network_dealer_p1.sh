#!/bin/bash

# AppleScript inside shell to open new Terminal windows and run commands
osascript <<EOF
tell application "Terminal"
    activate

    -- Window 1
    set w1 to do script "cd ~/CODE/GraphMPC/build/tests && ./test_network -p 1 --localhost"
    delay 0.2 
    set bounds of front window to {0, 0, 500, 800} -- {left, top, right, bottom}

    -- Window 2
    set w2 to do script "cd ~/CODE/GraphMPC/build/tests && ./test_network -p 2 --localhost"
    delay 0.2 
    set bounds of front window to {500, 0, 1000, 800}
    
end tell
EOF