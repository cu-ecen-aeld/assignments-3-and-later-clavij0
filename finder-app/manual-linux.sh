#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
#KERNEL_REPO=https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
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

    #Deap Clean remove .config
    echo "Deap Clean remove .config"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    #Config our virt arm dev board -> will simulate in QEMU
    echo "Config our virt arm dev board -> will simulate in QEMU"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    #Build a Kernel image for booting with QEMU
    echo "Build a Kernel image for booting with QEMU"
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
    #Modules and deviceTree
    echo "No Modules installed in this assigment 3-2"
    #make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    echo "DeviceTree"
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs  
    
fi

echo "Adding the Image in outdir"
if [ -d "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" ]
then
rm -r Image
fi

cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    rm  -rf ${OUTDIR}/rootfs
fi

echo "Create rootfs directory ${OUTDIR}/rootfs"
mkdir -p ${OUTDIR}/rootfs
# TODO: Create necessary base directories
cd "$OUTDIR"/rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var conf
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    echo "Inside busybox"
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    echo "TODO:  Configure busybox"
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
echo "make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install"
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

cd "$OUTDIR"/rootfs
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs -> Place the lib in /lib64
echo "TODO: Add library dependencies to rootfs"
#cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib64
cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64

cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ${OUTDIR}/rootfs/lib
cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib
cp ~/Documents/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ${OUTDIR}/rootfs/lib

# TODO: Make device nodes
echo "TODO: Make device nodes"
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
#sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 1 5

# TODO: Clean and build the writer utility
echo "TODO: Clean and build the writer utility"
cd ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/
make

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "TODO: Copy the finder related scripts and executables to the /home directory \n on the target rootfs"
cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/writer ${OUTDIR}/rootfs/home
cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/finder.sh ${OUTDIR}/rootfs/home
#cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/conf/username.txt ${OUTDIR}/rootfs/home
mkdir ${OUTDIR}/rootfs/home/conf
cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/conf/username.txt ${OUTDIR}/rootfs/home/conf
cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/finder-test.sh ${OUTDIR}/rootfs/home
cp ~/Documents/Coursera/LinuxSPIBuildroot/finder-app/autorun-qemu.sh ${OUTDIR}/rootfs/home

# TODO: Chown the root directory
echo "TODO: Chown the root directory"
cd "$OUTDIR"/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio

cd "$OUTDIR"
# TODO: Create initramfs.cpio.gz
if [ -f initramfs.cpio.gz ]
then
    rm -r initramfs.cpio.gz
fi
echo "TODO: Create initramfs.cpio.gz"
gzip -f initramfs.cpio

