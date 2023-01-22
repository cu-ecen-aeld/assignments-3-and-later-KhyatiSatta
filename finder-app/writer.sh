#ECEN-5713 Advanced Embedded Software Development
#Author name: Khyati Satta
#Date: 01/22/2023
#File Description: Writer script to write a given expression to a file

#!/bin/bash

#Store the new text file name and the expression to be written
text_file=$1
expr=$2

#Error check 1: Check if number of arguments is 2
if [ $# -ne 2 ]
then
    echo Please enter both arguments
    exit 1
fi

#Extract the directory name from the file path. Reference: https://www.cyberciti.biz/faq/unix-get-directory-name-from-path-command/
#Create the file and write the given expression using the echo command
mkdir -p "$(dirname "$text_file")" && echo "$expr" > "$text_file"

exit 0

