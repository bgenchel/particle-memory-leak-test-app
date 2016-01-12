#!/bin/bash
source ~/.bash_profile

src_dir=/Users/macbook/Documents/Cleverpet/Cleverpet_BehaviorLayer
dest_dir=$src_dir/firmware/user/applications
run_dir=$src_dir/firmware/main

appname=test-tcp-client

echo "Copying application into 'applications' folder.."
rm -rf $dest_dir/$appname
cp -rf $src_dir/$appname $dest_dir/
echo "Application copied successfully."

cd $run_dir
if [ $# -eq 0 ] 
then
    make PLATFORM=photon APP=$appname program-dfu
elif [ $1 = "-clean" ]
then
    make clean all PLATFORM=photon APP=$appname program-dfu
else
    echo "invalid args, terminating."
fi
