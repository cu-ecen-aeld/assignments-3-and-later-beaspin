#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=${1:-/tmp/aeld}
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-

echo "Using OUTPUT directory: ${OUTDIR}"
ABS_OUTDIR=$(realpath "${OUTDIR}")

mkdir -p ${ABS_OUTDIR} || { echo "Failed to create output directory"; exit 1; }

REQUIRED_TOOLS=("make" "gcc" "git" "wget" "tar" "${CROSS_COMPILE}gcc" "cpio")
for tool in "${REQUIRED_TOOLS[@]}"; do
    if ! command -v "$tool" &>/dev/null; then
        echo "Error: $tool is not installed. Please install it and retry."
        exit 1
    fi
done

cd "${ABS_OUTDIR}"
if [ ! -d "linux-stable" ]; then
    echo "Cloning Linux kernel source..."
    git clone --depth 1 --single-branch --branch ${KERNEL_VERSION} ${KERNEL_REPO} linux-stable
fi

cd linux-stable
echo "Checking out version ${KERNEL_VERSION}"
git checkout ${KERNEL_VERSION}

if [ ! -e ${ABS_OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    echo "Building the kernel..."
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

if [ -f "/tmp/aesd-autograder/linux-stable/arch/arm64/boot/Image" ]; then
    echo "Kernel Image found. Copying..."
    cp /tmp/aesd-autograder/linux-stable/arch/arm64/boot/Image /tmp/aeld/Image
else
    echo "Kernel Image is missing. Built might have failed."
    exit 1
fi

ROOTFS_DIR="${OUTDIR}/rootfs"
if [ -d "${ROOTFS_DIR}" ]; then
    echo "Cleaning existing rootfs directory..."
    sudo rm  -rf "${ROOTFS_DIR}"
fi

mkdir -p "${ROOTFS_DIR}"/{bin,sbin,etc,proc,sys,usr/bin,usr/sbin,lib,lib64,dev,home,tmp,var,root}

cd "${ABS_OUTDIR}"
if [ ! -d "busybox" ]; then
    echo "Cloning BusyBox..."
    git clone git://busybox.net/busybox.git
fi

cd busybox
git fetch --tags
git checkout master
make distclean
make defconfig
make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="${ROOTFS_DIR}" install

echo "Adding library dependencies..."
SYSROOT="/usr/aarch64-linux-gnu/"

if [ -d "${SYSROOT}/lib" ]; then
    LIB_DIR="${SYSROOT}/lib"
elif [ -d "${SYSROOT}/lib64" ]; then
    LIB_DIR="${SYSROOT}/lib64"
else
    echo "Error: No valid lib directory found in ${SYSROOT}"
    exit 1
fi

mkdir -p "${ROOTFS_DIR}/lib"
mkdir -p "${ROOTFS_DIR}/lib64"
cp -a ${LIB_DIR}/ld-linux-aarch64.so.1 "${ROOTFS_DIR}/lib/"
cp -a ${LIB_DIR}/libm.so.6 "${ROOTFS_DIR}/lib/"
cp -a ${LIB_DIR}/libresolv.so.2 "${ROOTFS_DIR}/lib/"
cp -a ${LIB_DIR}/libc.so.6 "${ROOTFS_DIR}/lib/"

echo "Creating device nodes..."
sudo mknod -m 666 "${ROOTFS_DIR}/dev/null" c 1 3
sudo mknod -m 600 "${ROOTFS_DIR}/dev/console" c 5 1

cd "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

echo "Copying Finder App and scripts..."
mkdir -p "${ROOTFS_DIR}/home"
cp "${FINDER_APP_DIR}/writer" "${ROOTFS_DIR}/home/"
cp "${FINDER_APP_DIR}/conf/username.txt" "${ROOTFS_DIR}/home/"
cp "${FINDER_APP_DIR}/conf/assignment.txt" "${ROOTFS_DIR}/home/"
cp "${FINDER_APP_DIR}/finder-test.sh" "${ROOTFS_DIR}/home/"
sed -i 's|../conf/assignment.txt|conf/assignment.txt|' "${ROOTFS_DIR}/home/finder-test.sh"
cp "${FINDER_APP_DIR}/autorun-qemu.sh" "${ROOTFS_DIR}/home/"

echo "Setting root owenship for rootfs..."
sudo chown -R root:root "${ROOTFS_DIR}"

echo "Creating initramfs..."
cd "${ROOTFS_DIR}"
find . | cpio -o --format=newc | gzip > "${ABS_OUTDIR}/initramfs.cpio.gz"

echo "Kernel and root filesystem build complete!"
