#!/bin/bash

ARGS=2        # Number of arguments expected.
E_BADARGS=65  # Exit value if incorrect number of args passed.
msg="Usage: `basename $0` {file_name} #{project_dir}" 

# Make sure args were give
test $# -ne $ARGS && echo $msg && exit $E_BADARGS

project_name=$2
source_dir=/var/data/dev/source/active/$project_name

# Check that the file is a regular file and exists
if [ ! -f $1 ]
then
    echo "Error: file $1 does not exist"
    exit 1
fi

# Read the file
while read curline; do
    set -- $curline
    echo $1 '->' $2

    # Compute the target directory. If $2 does not end in a slash, it is a
    # file. In that case, we went the file's direname.
    if [[ $2 != */ ]] 
    then
        dir=`dirname $2`
    else
        # Else this is a directory
        dir=$2
    fi

    # Now check that taget directory exists
    if [ ! -d $dir ]
    then
        # Doesn't. So make it.
        mkdir -p $dir
    fi

    # Create symlink
    ln -sf $source_dir/$1 $2
done < "$1"
