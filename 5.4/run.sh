#!/bin/bash
name=$(echo $(hostname) | awk -F'.' '{print $1}')
python tests.py > $name/$name.csv
python graph.py $name/$name.csv