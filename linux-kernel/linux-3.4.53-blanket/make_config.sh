#!/bin/bash

cp /boot/config-$(uname -r) .config
cat recommend_config.yes >> .config
yes "" | make oldconfig
