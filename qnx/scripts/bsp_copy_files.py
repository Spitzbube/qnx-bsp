#!/usr/bin/env python
###############################################################################
#                                                                             #
# Copyright (c) 2024, Texas Instruments Incorporated                          #
# SPDX License Identifier: MIT                                                #
# Name: bsp_copy_files.py                                                     #
#                                                                             #
# This is a helper tool used to copy related bsp files from the soc specific  #
# bsp folder to the scripts bsp directory, so that the install script can     #
# patch the end user directory with TI specific changes.                      #
#                                                                             #
# To see Usage:                                                               #
#   python3  bsp_copy_files.py -h                                             #
#                                                                             #
# Sample command used in TI PSDK QNX Build environment                        #
#   python3  bsp_copy_files.py    ini file specific to the bsp and soc        #
#                                  psdkqa/scripts/bsp folder                  #
#                                                                             #
# Expected output:                                                            #
#      The files that need to be patched on top of the official QNX BSP for   #
#      the specific SOC would be copied into the psdkqa/bsp/<BSP version>     #
#                                                                             #
###############################################################################
import sys
import os
import re
import ast
import shutil
from pprint import pprint
import argparse
import subprocess
import platform
import configparser

from os import chdir as cd
from os.path import exists

os_name = platform.system()

def handleFoldersSection(config):
    # Get the list of dirs to create and copy
    dirCreateCfg = config.get('folders', 'create')
    dirCopyCfg = config.get('folders', 'copy')

    # Get the source path and check if it is valid
    bspRepoCfg = config.get('bsp', 'repo')
    srcBspPath = os.path.join(os.environ.get('PSDK_QNX_PATH'), "qnx", bspRepoCfg)

    if not os.access(srcBspPath, os.R_OK):
        print("ERROR: invalid source bsp path check configuration" + srcBspPath)
        exit(1)

    # Get the destination path
    bspVersionCfg = config.get('bsp', 'version')
    bspPath = os.path.join(os.getcwd(), "bsp", bspVersionCfg)

    #Create all directories
    print("Creating Directories...")
    for line in dirCreateCfg.splitlines():
        dirPath = os.path.join(bspPath, line)
        print(dirPath)
        os.makedirs(dirPath, exist_ok=True)
    print("...done")

    # Copy all directories
    print("Copying Directories...")
    # NOTE:: The line SHOULD NOT HAVE a '/" at the end of its path
    for line in dirCopyCfg.splitlines():
        dstPath = str((os.path.join(bspPath, line))).rsplit('/',1)[0]
        srcPath= os.path.join(srcBspPath, line)
        subprocess.run(['cp', '-axv', srcPath, dstPath])
    print("...done")


def handleFilesSection(config):

    # Get the list of files to copy and delete
    fileAddCfg = config.get('files', 'add')
    fileDelCfg = config.get('files', 'delete')

    # Get the source path and check if it is valid
    bspRepoCfg = config.get('bsp', 'repo')
    srcBspPath = os.path.join(os.environ.get('PSDK_QNX_PATH'), "qnx", bspRepoCfg)

    if not os.access(srcBspPath, os.R_OK):
        print("ERROR: invalid source bsp path check configuration" + srcBspPath)
        exit(1)

    # Get the destination path
    bspVersionCfg = config.get('bsp', 'version')
    bspPath = os.path.join(os.getcwd(), "bsp", bspVersionCfg)

    # Copy files
    print("Copying files...")
    for line in fileAddCfg.splitlines():
        dstPath = os.path.join(bspPath, line)
        srcPath= os.path.join(srcBspPath, line)
        #print(srcPath)
        #print(dstPath)
        #print("++++++")
        subprocess.run(['cp', '-axv', srcPath, dstPath])
    print("...done")



def handleBSPSection(config):

    bspVersionCfg = config.get('bsp', 'version')
    bspPath = os.path.join(os.getcwd(), "bsp", bspVersionCfg)

    os.makedirs(bspPath, exist_ok=True)

# Parse the configFile and get sections
def ParseConfigFile(configFile):
    config=configparser.ConfigParser()

    iniPath = os.path.join(os.getcwd(), "cfgs", configFile)
    config.read(iniPath)

    config.sections()

    #Handle sections
    handleBSPSection(config)
    handleFoldersSection(config)
    handleFilesSection(config)

    #its = config.get('files', 'add')
    #print(its)


# Main function of the bsp_copy_files.py file
# Takes in all of the arguments from the arg parse to clone the repo to the psdkra_dir from the repo_manifest
# Input: ini file specific to the soc and bsp .
if __name__ == "__main__":

    # Parse the input parameter
    parser = argparse.ArgumentParser()

    parser.add_argument('--configFile', dest='configFile', help='config file present in this directory for the BSP of interest')

    # Invoke the parser
    args = vars(parser.parse_args())

    if args["configFile"] is None:
        print("ERROR: No config file provided")
        exit(1)

    if os.environ.get('PSDK_QNX_PATH') is None:
        print("ERROR: Please set a valid PSDK_QNX_PATH")
        exit(1)

    ParseConfigFile(args["configFile"])

