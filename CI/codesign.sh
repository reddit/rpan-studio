#!/bin/bash

IDENTITY="$1"
APP_NAME="RPAN Studio.app"

find "${APP_NAME}" -type f -perm 755 -exec codesign -v -s ${IDENTITY} --deep --force -o runtime --entitlements "entitlements-browser.plist" {} \;
find "${APP_NAME}" -type f -perm 755 -exec codesign -v {} \;
codesign -v -s ${IDENTITY} --force "${APP_NAME}" -o runtime --entitlements "entitlements-browser.plist"
