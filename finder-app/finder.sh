#ECEN-5713 Advanced Embedded Software Development
#Author name: Khyati Satta
#Date: 01/22/2023
#File Description: Finder script to find number of files in a given directory containing a given expression

#!/bin/bash

#Store the first(the directory) and second(the string to search in the files)command line arguments into variables 
direc=$1
exp=$2

#Variables for storing the output: Number of files in the directory and the number of matching lines in the files
n_files=0
n_lines=0

#Error check 1: Check if number of arguments is 2
if [ $# -ne 2 ]
then
    echo Please enter both arguments
    exit 1
fi

#Error check 2: Check if the directory is valid with -d flag
if [ ! -d "$direc" ]
then
    echo "Invalid directory "$direc""
    exit 1
fi

#Using grep command combined with the wc command, find files which have the matching string recursively(to cover all the sub-directories within the directory)
#Using the -r flag to search recursively within the directory in all the files
#Using the -l flag to print the files that have the expression
n_files=$(grep -r -l "$exp" "$direc" | wc -l )
n_lines=$(grep -r "$exp" "$direc" | wc -l )

echo "The number of files are $n_files and the number of matching lines are $n_lines "

exit 0

    



