#!/bin/bash

# deps.
rm -rf deps.tar.gz
tar -cvf deps.tar.gz ../deps

# grps.
rm -rf grps.tar.gz grps
mkdir grps
cp -r ../apis ../grpst/ ../server/ ../template/ ../grps_*.sh ./grps/
tar -cvf grps.tar.gz grps

# client.
rm -rf client.tar.gz
mkdir client
cp -r ../apis ../grps_client_*.sh ./client/
tar -cvf client.tar.gz client