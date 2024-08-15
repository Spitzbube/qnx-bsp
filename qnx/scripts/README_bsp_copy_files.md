
This document captures the general approach to creating the packaging specific
BSP folder for each SoC that goes out with the PSDK QNX release.

The reason this is required is due to the fact that TI makes certain changes
on top of the the vanilla QNX BSP release. These changes are either to suit
TIs needs in enhancing the BSP to support TI specific use cases or fixes to
the QNX BSP that requires additional time, effort, and cost to get those
merged into the QNX upstream BSP.


The QNX BSP released to TI/QSC is generally a zip file that sits in the
~/qnx<BSP_VERSION>/qnx/bsp folder. For instance, here is a snapshot of BSPs
installed in a developer's system:

```

ls -1  ~/qnx800/bsp/
BSP_ti-j721e-tda4vm-sk_br-hw-rel_be-800_SVN999670_JBN205.zip
BSP_ti-j721s2-tda4vmeco-evm_br-hw-rel_be-800_SVN996727_JBN40.zip
BSP_ti-j721s2-tda4vmeco-evm_br-hw-rel_be-800_SVN998918_JBN91.zip
BSP_ti-j722s-evm_br-hw-rel_be-800_SVN997772_JBN64.zip
BSP_ti-j784s4-evm_br-hw-rel_be-800_SVN993440_JBN2.zip
BSP_ti-j784s4-evm_br-hw-rel_be-800_SVN997851_JBN73.zip

```

Taking BSP_ti-j784s4-evm_br-hw-rel_be-800_SVN997851_JBN73.zip as an example,
the following steps help create a configuration file that can be used to
copy relevant files to a script.

- Extract the zip folder to a temporary location. For instance: /tmp/800_j784s4_bsp/
- Make sure your repo is in a clean state and remove all untracked contents inthe bsp repo.
    Usually, git clean -dfx should remove all untracked files.
- Compare the contents with the bsp in the repo. In this case, psdkqa/bsp/bsp_800_j784s4
- Create a cfg file that can be used to generate the final cfg file.
- Execute the script from the psdkqa/qnx/scripts/ directory as follows:

    `
    cd psdkqa/qnx/scripts/
    PSDK_QNX_PATH=/data/workarea/j7/psdkqa/ python3 bsp_copy_files.py --configFile j784s4_800_bsp.ini
    `

Here are some handy commands to get the list of files:

# Diff between QNX bsp and TI bsp

## Files that differ
# Find all files/folders that differ between the 2 bsps
`diff -qr . /tmp/bsp-j721e/bsp/ --suppress-common-lines | grep  -E -v "\.git" | grep -iE "Only in \." | cut -d ' ' -f 3- >> /tmp/diff.files`
# Find files/folders that are present only in the TI specific BSP
`diff -qr . /tmp/bsp-j721e/bsp/ --suppress-common-lines | grep  -E -v "\.git" | grep -iE "Only in \." | cut -d ' ' -f 3- >> /tmp/diff.files`

The diff.files will now have only the subset of items that we are interested in. Replace ": " in the "Only in " lines with "/" to get the
files/folders path that are of interest. Ignore git related files, cpsw/asix build files and such to arrive at the file list.

### Sample output
/images/Makefile
/Makefile


## Files that are new in TI BSP

`diff -qr . /tmp/800_j784s4_bsp/ --suppress-common-lines | grep -v tmp`

### Sample output

Only in .: etc
Only in .: .git
Only in .: .gitignore
Only in ./images: j784s4-evm-ti.build
Only in ./images: j784s4-evm-ti-spl-nfs-with-asix.build
Only in ./install: .timbits
Only in .: j784s4_config.mk


- NOTE:: j784s4-evm-ti-spl-nfs-with-asix.build and other git files are ignored.
- NOTE:: etc is a directory

Based on the above 2 output, the final cfg would look something like this:


[bsp]
version = BSP_ti-j784s4-evm_br-hw-rel_be-800_SVN997851_JBN73
repo    = bsp_800_j784s4

[folders]

create = etc
         images

copy = etc

[files]
add =   j784s4_config.mk
        Makefile
        images/Makefile
        images/j784s4-evm-ti.build

delete =
#END
