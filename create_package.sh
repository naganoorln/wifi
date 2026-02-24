#!/bin/bash

APP_NAME="wifi-manager"
VERSION="1.0-1"
ARCH="amd64"

echo "Cleaning old files..."
rm -rf ${APP_NAME}_${VERSION}
rm -f ${APP_NAME}_${VERSION}.deb
rm -f ${APP_NAME}

echo "Compiling..."
gcc wifi_manager.c -o ${APP_NAME}

if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

echo "Creating directory structure..."
mkdir -p ${APP_NAME}_${VERSION}/DEBIAN
mkdir -p ${APP_NAME}_${VERSION}/usr/bin

echo "Copying binary..."
cp ${APP_NAME} ${APP_NAME}_${VERSION}/usr/bin/

echo "Creating control file..."
cat <<EOF > ${APP_NAME}_${VERSION}/DEBIAN/control
Package: ${APP_NAME}
Version: ${VERSION}
Section: net
Priority: optional
Architecture: ${ARCH}
Maintainer: Lakshman
Depends: libc6
Description: Interactive WiFi CLI Manager
 A simple command-line WiFi management tool.
EOF

echo "Setting permissions..."
chmod 755 ${APP_NAME}_${VERSION}/usr/bin/${APP_NAME}
chmod 755 ${APP_NAME}_${VERSION}/DEBIAN
chmod 644 ${APP_NAME}_${VERSION}/DEBIAN/control

echo "Building .deb package..."
dpkg-deb --build ${APP_NAME}_${VERSION}

echo "Done!"
echo "Package created: ${APP_NAME}_${VERSION}.deb"
