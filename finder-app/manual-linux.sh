#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # Reference: Lecture slides

    #Deep clean the kernel
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper

    #Configure for 'virt' ARM dev board (QEMU)
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig

    #Build kernel image for booting with QEMU
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

    #Build all the modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    #Build the device tree
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
# Reference: Lecture slides
mkdir ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    # Reference: Lecture slides
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make -j4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd ${OUTDIR}/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
echo "$SYSROOT"

cp -a $SYSROOT/lib/ld-linux-aarch64.so.1 lib
cp -a $SYSROOT/lib64/ld-2.31.so lib64
cp -a $SYSROOT/lib64/libc.so.6 lib64
cp -a $SYSROOT/lib64/libc-2.31.so lib64
cp -a $SYSROOT/lib64/libm.so.6 lib64
cp -a $SYSROOT/lib64/libm-2.31.so lib64
cp -a $SYSROOT/lib64/libresolv.so.2 lib64
cp -a $SYSROOT/lib64/libresolv-2.31.so lib64
echo "Added the required library dependencies"

# TODO: Make device nodes
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
echo "Created node devices"

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
echo "Built writer utility"

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
# Copy the writer script, executable and associated Makefile
cp ${FINDER_APP_DIR}/writer.c ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/Makefile ${OUTDIR}/rootfs/home

#Copy the finder script, executable
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home

#Copy tyhe entire conf directory
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home

#Copy autorun-qemu script
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home

echo "Copied all the finder-app scripts"

# TODO: Chown the root directory
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

echo "Chowned the root directory"

# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ../initramfs.cpio
cd ..
gzip -f initramfs.cpio

echo "Created initramfs.cpio.gz"