#!/bin/bash
export LD_LIBRARY_PATH=/usr/share/SunnyCapturer/lib:$LD_LIBRARY_PATH
export QT_PLUGIN_PATH=/usr/share/SunnyCapturer/plugins
exec /usr/bin/SunnyCapturer "$@"

