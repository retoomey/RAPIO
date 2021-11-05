#!/bin/bash
# Toomey July 2021
# Just a simple copy/rename of TEMPLATE to a given directory
# You can of course also do it by hand, etc.

if [ "$#" -ne 1 ]; then
  echo "Give a directory/alg name for your new algorithm. Recommend CamelCase such as 'SevereAlg' or 'HailAlg', etc."
  exit
fi

echo "Trying to copy TEMPLATE..."
if [ -d $1 ]; then
  echo "...refusing to overwrite existing directory $1."
  exit
fi
if [ ! -d "TEMPLATE" ]; then
  echo "...don't see the TEMPLATE directory here.  I need it to make the new algorithm."
  exit
fi
cp -r TEMPLATE $1

if [ -d $1 ]; then
  echo "...successfully copied.  Trying to modify template to your algorithm name."
fi

echo "Entering directory $1..."
cd $1

echo "Modifying template files..."
SUB="s/TEMPLATE/$1/g"
sed -i $SUB MakefileTemplate
mv MakefileTemplate Makefile
sed -i $SUB runme.sh
sed -i $SUB rTEMPLATE.h
mv rTEMPLATE.h r$1.h
sed -i $SUB rTEMPLATE.cc
mv rTEMPLATE.cc r$1.cc

echo "Type make in $1 to build your algorithm (on the container)"
