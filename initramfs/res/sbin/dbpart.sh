#!/sbin/sh
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Library General Public License as published
# by the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>
#
# Written by:
# Rodderik <rodderik@devphone.org>
# http://devphone.org
#
# sdparted v0.6 by 51dusty was a big help in writing this script. Thanks 51dusty!
#
# Version 0.1 - initial version
#

#################
# BEGIN OPTIONS #
#################
#for CWM ui output
OUTFD=$(ps | grep -v "grep" | grep -o -E "update_binary(.*)" | cut -d " " -f 3);

#define partition sizes in MB
SYSTEMSIZE="280"
DATASIZE="530"
CACHESIZE="180"
SDPATH="/dev/block/mmcblk0"

#define partition names
SDNAME="SDCARD"
SYSTEMNAME="SYSTEM"
DATANAME="DATA"
CACHENAME="CACHE"

#partition order (do not change these)
#fat partition MUST be first
SYSTEMPART="p2"
DATAPART="p3"
CACHEPART="p4"

#file system specific options
FILESYSTEM="ext4"
EXT4OPTS="^resize_inode,^ext_attr,^huge_file"
FATOPTS="-S 2048 -s 1 -F 32"
FATOPTSCACHE="-S 4096 -s 1 -F 16"

#take care of binaries
PARTEDBIN="/sbin/parted"
FATBIN="/sbin/fat.format"
MEXT4BIN="/sbin/mkfs.ext4"
TEXT4BIN="/sbin/tune2fs.ext4"

#script options
LOGFILE="/data/dbpart.log"
SUPRESS=
DRYRUN=

###############
# END OPTIONS #
###############

# do logging, if not excluded with -x
[ "$1" != "-x" ] && echo "$0" "$@" >> "$LOGFILE" && "$0" -x "$@" 2>&1 | tee -a "$LOGFILE" && exit
shift

DISPHELP () {
cat <<DONEDISPHELP
${SCRIPTNAME} v${SCRIPTVER}
Brought to you by ${SCRIPTAUTH}

This script aids in preparing an sdcard for dual booting with supported
kernels.

Requirements:
Sdcard 2 gigabytes or larger
Dual boot supported kernel
Tested with parted 1.8.8.1.179-aef3 compiled for android.

Usage:  ${SCRIPTNAME} [options]
    
Options:

 -fs|--filesystem    Sets desired file system for sdcard partitions.
	             Valid types: rfs, ext4
                     Default is 'ext4'.

 -s|--supress        Supress all information and user prompts.  Warnings are
                     still displayed.  Useful for calling script externally.
                     Default option is to NOT suppress information and prompts.
	
 -dr|--dryrun        Performs a dry run of the script that will show proposed
                     partition sizes without making any changes to the actual
                     sdcard. Automatically ignores --supress option.
                     Default option is to NOT do a dry run.

 -h|--help           Display this help menu.

Examples:
${SCRIPTNAME}               Partitions sdcard default options.
${SCRIPTNAME} -fs rfs       Partitions sdcard with rfs and default options.
${SCRIPTNAME} -dr -fs rfs   Performs dry run and displays proposed partition
                            information using rfs file system.
${SCRIPTNAME} -s            Supress all user input and information. This WILL
                            DESTROY existing partitions the sdcard with out any
                            warning or conformation.

DONEDISPHELP
}

SHOWERROR() { 
echo1
echo1 "Error: $1"
echo1
exit 1
}

ECHOIT() { 
if [ -z $SUPRESS ]; then
    echo1 "Info: $1"
fi
}

echo1() {
  if [ ${OUTFD} ]; then
    echo "ui_print ${1} " 1>&$OUTFD;
    echo "ui_print " 1>&$OUTFD;
  else
    if [ "${#}" -lt 1 ]; then
	echo " ";
    else
	echo "${1}";
    fi;
  fi;
}

BINCHK () {
#check for binaries
if [ ! -e $PARTEDBIN ] && [ -f $PARTEDBIN ]; then
    SHOWERROR "${PARTEDBIN} is missing."
fi
if [ ! -e $FATBIN ] && [ -f $FATBIN ] && [ $FILESYSTEM == "rfs" ]; then
    SHOWERROR "${FATBIN} is missing."
fi
if [ ! -e $MEXT4BIN ] && [ -f $MEXT4BIN ] && [ $FILESYSTEM == "ext4" ]; then
    SHOWERROR "${MEXT4BIN} is missing."
fi
if [ ! -e $TEXT4BIN ] && [ -f $TEXT4BIN ] && [ $FILESYSTEM == "ext4" ]; then
    SHOWERROR "${TEXT4BIN} is missing."
fi
}

RUNSTRING () {
#construct run string
if [ -n $SUPRESS ]; then
    DOPARTED="${PARTEDBIN} -s ${SDPATH}"
else 
    DOPARTED="${PARTEDBIN} ${SDPATH}"
fi
}

FORMATEXT4 () {
#format ext4
ECHOIT "Formatting system"
$MEXT4BIN -O ${EXT4OPTS} -L ${SYSTEMNAME} -J size=16 -b 4096 -m 0 -F -N 7500 ${SDPATH}${SYSTEMPART} > /dev/null 2>&1 >>"$LOGFILE"
$TEXT4BIN -O +has_journal -c 30 -i 30d -m 0 -o journal_data_writeback ${SDPATH}${SYSTEMPART} 2>&1 >>"$LOGFILE"

ECHOIT "Formatting data"
$MEXT4BIN -O ${EXT4OPTS} -L ${DATANAME} -J size=16 -b 4096 -m 0 -F -N 7500 ${SDPATH}${DATAPART} > /dev/null 2>&1 >>"$LOGFILE"
$TEXT4BIN -O +has_journal -c 30 -i 30d -m 0 -o journal_data_writeback ${SDPATH}${DATAPART} 2>&1 >>"$LOGFILE"

ECHOIT "Formatting cache"
$MEXT4BIN -O ${EXT4OPTS} -L ${CACHENAME} -J size=16 -b 4096 -m 0 -F -N 7500 ${SDPATH}${CACHEPART} > /dev/null 2>&1 >>"$LOGFILE"
$TEXT4BIN -O +has_journal -c 30 -i 30d -m 0 -o journal_data_writeback ${SDPATH}${CACHEPART} 2>&1 >>"$LOGFILE"
}

FORMATRFS () {
#format rfs
ECHOIT "Formatting system"
$FATBIN $FATOPTS ${SDPATH}${SYSTEMPART} 2>&1 >>"$LOGFILE"

ECHOIT "Formatting data"
$FATBIN $FATOPTS ${SDPATH}${DATAPART} 2>&1 >>"$LOGFILE"

ECHOIT "Formatting cache"
$FATBIN $FATOPTSCACHE ${SDPATH}${CACHEPART} 2>&1 >>"$LOGFILE"
}

UNMOUNTPARTS () {
mount | grep /mmcblk | cut -d" " -f1 | while read line; do 
    ECHOIT "Unmounting ${line}"  
    umount -f $line 2>&1 >>"$LOGFILE"
done
}

NUKEPARTS () {
#nuke partitions
ECHOIT "Nuking existing partition(s)"
$DOPARTED rm 1 > /dev/null 2>&1
$DOPARTED rm 2 > /dev/null 2>&1
$DOPARTED rm 3 > /dev/null 2>&1
$DOPARTED rm 4 > /dev/null 2>&1
}

GETSDINFO () {
#get sdcard size
SDSIZEMB=`${DOPARTED} unit MB print | grep ${SDPATH} | cut -d" " -f3`
SDSIZE=${SDSIZEMB%MB}
}

CALCLAYOUT () {
#calculate partition layout
FPARTEND=$(($SDSIZE - $SYSTEMSIZE - $DATASIZE - $CACHESIZE))
SPARTEND=$(($FPARTEND + $SYSTEMSIZE))
DPARTEND=$(($SPARTEND + $DATASIZE))
CPARTEND=$(($DPARTEND + $CACHESIZE))
}

CREATEPARTS () {
#create partitions
ECHOIT "Creating partitions. This may take a while."
ECHOIT "Creating partition 1."
$DOPARTED mkpartfs primary fat32 0 ${FPARTEND}MB 2>&1 >>"$LOGFILE"
ECHOIT "Creating partition 2."
$DOPARTED mkpartfs primary ext2 ${FPARTEND}MB ${SPARTEND}MB 2>&1 >>"$LOGFILE"
ECHOIT "Creating partition 3."
$DOPARTED mkpartfs primary ext2 ${SPARTEND}MB ${DPARTEND}MB 2>&1 >>"$LOGFILE"
ECHOIT "Creating partition 4."
$DOPARTED mkpartfs primary ext2 ${DPARTEND}MB ${CPARTEND}MB 2>&1 >>"$LOGFILE"
}

SANITYCHECKS () {
#check sdcard minimum size
if [ $SDSIZE -le "1800" ]; then
    SHOWERROR "Could not read sdcard size correctly or sdcard is less than 2 gigabytes in size."
fi

if [ $SDSIZE -lt $CPARTEND ]; then
    SHOWERROR "Error calculating partition sizes."
fi

if [ $FPARTEND -le 0 ]; then
    SHOWERROR "Fat partition must be greater than O megabytes."
fi
}

CHECKFS() {
# validating argument
if [ "$1" != "rfs" ] && [ "$1" != "ext4" ]; then
    SHOWERROR "$1 is not a valid filesystem."
else 
    FILESYSTEM="$1"
fi
}

USERCANCEL() {
ECHOIT "Canceled by user. Now exiting."
exit 0
}

SUPRESSCHK() {
if [ ! -z $DRYRUN ]; then
    SUPRESS=""
fi
}

SCRIPTNAME=dbpart.sh
SCRIPTVER="0.1"
SCRIPTAUTH="Rodderik (rodderik@devphone.org)"

# command line parsing
while [ $# -gt 0 ]; do
  case "$1" in

    -h|--help) DISPHELP ; exit 0 ;;

    -fs|--filesystem) shift ; CHECKFS "$1" ;;

    -s|--supress) SUPRESS="$1" ;;

    -dr|--dryrun) DRYRUN="$1" ;;

    *) DISPHELP ; SHOWERROR "Unknown command-line argument '$1'" ;;

  esac
  shift
done


#do stuff right meow!
#disable supression for dry run
SUPRESSCHK

# give the output some breathing room
echo "-------------------------------------------------------------------------------" >> "$LOGFILE" 

#check for binaries
BINCHK

#construct run string
RUNSTRING

#snag sdcard information
GETSDINFO

#calculate new layout
CALCLAYOUT

#double check ourselves
SANITYCHECKS

if [ -z $SUPRESS ]; then
    echo1 "${SCRIPTNAME} v${SCRIPTVER}"
    echo1 "Brought to you by ${SCRIPTAUTH}"
    echo1
    echo1 "Disk ${SDPATH}: ${SDSIZEMB}"
    echo1
    echo1 "I am going to make the following changes to your sdcard:"
    echo1
    echo1 "Partition 1: Name: ${SDNAME} File System: fat32 Size: ${FPARTEND}MB"
    echo1 "Partition 2: Name: ${SYSTEMNAME} File System: ${FILESYSTEM} Size: ${SYSTEMSIZE}MB"
    echo1 "Partition 3: Name: ${DATANAME} File System: ${FILESYSTEM} Size: ${DATASIZE}MB"
    echo1 "Partition 4: Name: ${CACHENAME} File System:  ${FILESYSTEM} Size: ${CACHESIZE}MB"
    echo1
    if [ ${OUTFD} ]; then
	echo1 "Script initiated from Clockwork Mod with NO confirmation.  I hope you read before flashing this."
        echo1
    else
        if [ -z $DRYRUN ]; then
            echo1 "WARNING!!! THIS WILL DESTROY ALL DATA ON SDCARD!!!  Commit changes?"
            read -s -n3 -p "Type YES to confirm, any thing else to cancel. " key
            case $key in
            "YES" | "yes") ;;
            *) USERCANCEL;;
            esac
        fi
    fi
fi

if [ -z $DRYRUN ]; then
    UNMOUNTPARTS
    NUKEPARTS
    CREATEPARTS
    if [ $FILESYSTEM == "ext4" ]; then
        FORMATEXT4
    elif [ $FILESYSTEM == "rfs" ]; then
        FORMATRFS
    else
        SHOWERROR "Unable to select filesystem type."
    fi
    ECHOIT "Operation seems to have completed successfully."
fi
exit 0
