#!/bin/bash

curl   -s -H "Accept: application/vnd.github.v3+json"   https://api.github.com/repos/ryaniftron/clearview_interface_public/releases | grep -E 'download_count|download_url' | sed 's/"//g' | sed 's/https:\/\/github.com\/ryaniftron\/clearview_interface_public\/releases\/download\/v1.2[0-9].[a][0-9]\///g'
