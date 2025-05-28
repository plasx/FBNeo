#!/bin/bash

TARGET="src/burner/metal/fixes/c_linkage_bridge.cpp"

# Remove QSound stub implementations in MetalBridgeFixes namespace
sed -i.bak \
    -e '/INT32 __QsndInit() { return 0; }/d' \
    -e '/void __QsndReset() { }/d' \
    -e '/void __QsndExit() { }/d' \
    -e '/void __QsndNewFrame() { }/d' \
    -e '/void __QsndEndFrame() { }/d' \
    -e '/void __QsndSyncZ80() { }/d' \
    -e '/INT32 __QsndScan(INT32 nAction) { return 0; }/d' \
    -e '/void __QsndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir) { }/d' \
    "$TARGET"

echo "QSound stub implementations removed from $TARGET (backup saved as $TARGET.bak)"
