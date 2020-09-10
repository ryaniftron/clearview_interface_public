#!/bin/bash

sed -r 's/\"/\\\"/g' $1 | sed '$a\'
