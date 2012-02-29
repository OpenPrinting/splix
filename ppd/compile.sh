#!/bin/sh
#
#       compile.sh                      (C) 2007, Aurélien Croc (AP²C)
#
#  Generate the DRV file to compile it by ppdc.
#  This script adds a new command "#import "file"" in the DRV file which imports
#  the content of the file where the command is located. To finish, it calls 
#  ppdc to compile the file and generate the PPD drivers.
#
# $Id$
# 

#
# Function parseFile
#
parseFile() {
    while read LINE; do
        if [ -n "`echo "$LINE" | grep '^[ \t]*#import[ \t]*"[a-zA-Z0-9\.\-]*"'`" ]; then
            FILE=`echo "$LINE" | sed -re 's/[ \t]*#import[ \t]"([a-zA-Z0-9\.\-]*)"/\1/'`
            parseFile $FILE $2
        else
            echo "$LINE" >> $2
        fi;
    done < $1
}


#
# Main script
#
if [ "$2" = "drv" ]; then
    DRIVER=$1
    OUTFILE=${DRIVER%.in}
    shift 1

    echo "" > $OUTFILE
    parseFile $DRIVER $OUTFILE


elif [ "$2" = "lang" ]; then
    if [ -z $TMP ]; then
        TMP="/tmp"
    fi;
    TMPFILE=`mktemp $TMP/driver.drv.XXXXXXXX` || exit 1
    DRIVER=$1

    echo "" > $TMPFILE
    parseFile $DRIVER $TMPFILE

    ppdpo -o $3 $TMPFILE
    unlink $TMPFILE


else
    if [ -z $TMP ]; then
        TMP="/tmp"
    fi;
    TMPFILE=`mktemp $TMP/driver.drv.XXXXXXXX` || exit 1
    DRIVER=$1
    shift 1

    echo "" > $TMPFILE
    parseFile $DRIVER $TMPFILE

    ppdc $@ $TMPFILE
    unlink $TMPFILE
fi;


# vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 enc=utf8:
