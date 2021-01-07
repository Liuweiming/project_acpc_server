#!/bin/bash
THIS_DIR=$( cd "$( dirname "$0" )" && pwd )
cd $THIS_DIR && ./example_player leduc.limit.2p.game $1 $2
