hr() {
  echo "───────────────────────────────────────────────────"
  echo $1
  echo "───────────────────────────────────────────────────"
}

# Exit if something fails
set -e

if [ -z "${CEF_ROOT_DIR}" ] ; then
    echo "Please set CEF_ROOT_DIR environment variable"
    exit
fi

if [ -z "${SIGN_IDENTITY}" ] ; then
    echo "Please set SIGN_IDENTITY environment variable"
    exit
fi

if [ -z "${BUILD_TYPE}" ] ; then
    BUILD_TYPE=RelWithDebInfo
fi
echo "Build type: ${BUILD_TYPE}"

# Generate file name variables
export GIT_TAG=$(git describe --abbrev=0)
export GIT_HASH=$(git rev-parse --short HEAD)
export FILE_DATE=$(date +%Y-%m-%d.%H-%M-%S)
export FILENAME=$FILE_DATE-$GIT_HASH-$TRAVIS_BRANCH-osx.dmg

echo "git tag: $GIT_TAG"

cd ./build

# Move obslua
if [ ! -f ./rundir/${BUILD_TYPE}/bin/obslua.so ] ; then
    hr "Moving OBS LUA"
    mv ./rundir/${BUILD_TYPE}/data/obs-scripting/obslua.so ./rundir/${BUILD_TYPE}/bin/
fi

# Move obspython
if [ ! -f ./rundir/${BUILD_TYPE}/bin/_obspython.so ] ; then
    hr "Moving OBS Python"
    #mv ./rundir/${BUILD_TYPE}/data/obs-scripting/_obspython.so ./rundir/${BUILD_TYPE}/bin/
    #mv ./rundir/${BUILD_TYPE}/data/obs-scripting/obspython.py ./rundir/${BUILD_TYPE}/bin/
fi

# Package everything into a nice .app
hr "Packaging .app"
STABLE=false
if [ -n "${TRAVIS_TAG}" ]; then
  STABLE=true
fi

#sudo python ../CI/install/osx/build_app.py --public-key ../CI/install/osx/OBSPublicDSAKey.pem --sparkle-framework ../../sparkle/Sparkle.framework --stable=$STABLE

../CI/install/osx-rpan/packageApp.sh

# fix obs outputs plugin it doesn't play nicely with dylibBundler at the moment
#if [ -f /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib ]; then
#    cp /usr/local/opt/mbedtls/lib/libmbedtls.12.dylib ./OBS.app/Contents/Frameworks/
#    cp /usr/local/opt/mbedtls/lib/libmbedcrypto.3.dylib ./OBS.app/Contents/Frameworks/
#    cp /usr/local/opt/mbedtls/lib/libmbedx509.0.dylib ./OBS.app/Contents/Frameworks/
#    chmod +w ./OBS.app/Contents/Frameworks/*.dylib
#    install_name_tool -id @executable_path/../Frameworks/libmbedtls.12.dylib ./OBS.app/Contents/Frameworks/libmbedtls.12.dylib
#    install_name_tool -id @executable_path/../Frameworks/libmbedcrypto.3.dylib ./OBS.app/Contents/Frameworks/libmbedcrypto.3.dylib
#    install_name_tool -id @executable_path/../Frameworks/libmbedx509.0.dylib ./OBS.app/Contents/Frameworks/libmbedx509.0.dylib
#    install_name_tool -change libmbedtls.12.dylib @executable_path/../Frameworks/libmbedtls.12.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#    install_name_tool -change libmbedcrypto.3.dylib @executable_path/../Frameworks/libmbedcrypto.3.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#    install_name_tool -change libmbedx509.0.dylib @executable_path/../Frameworks/libmbedx509.0.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#elif [ -f /usr/local/opt/mbedtls/lib/libmbedtls.13.dylib ]; then
#    cp /usr/local/opt/mbedtls/lib/libmbedtls.13.dylib ./OBS.app/Contents/Frameworks/
#    cp /usr/local/opt/mbedtls/lib/libmbedcrypto.5.dylib ./OBS.app/Contents/Frameworks/
#    cp /usr/local/opt/mbedtls/lib/libmbedx509.1.dylib ./OBS.app/Contents/Frameworks/
#    chmod +w ./OBS.app/Contents/Frameworks/*.dylib
#    install_name_tool -id @executable_path/../Frameworks/libmbedtls.13.dylib ./OBS.app/Contents/Frameworks/libmbedtls.13.dylib
#    install_name_tool -id @executable_path/../Frameworks/libmbedcrypto.5.dylib ./OBS.app/Contents/Frameworks/libmbedcrypto.5.dylib
#    install_name_tool -id @executable_path/../Frameworks/libmbedx509.1.dylib ./OBS.app/Contents/Frameworks/libmbedx509.1.dylib
#    install_name_tool -change libmbedtls.13.dylib @executable_path/../Frameworks/libmbedtls.13.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#    install_name_tool -change libmbedcrypto.5.dylib @executable_path/../Frameworks/libmbedcrypto.5.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#    install_name_tool -change libmbedx509.1.dylib @executable_path/../Frameworks/libmbedx509.1.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#fi
#
#install_name_tool -change /usr/local/opt/curl/lib/libcurl.4.dylib @executable_path/../Frameworks/libcurl.4.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change @rpath/libobs.0.dylib @executable_path/../Frameworks/libobs.0.dylib ./OBS.app/Contents/Plugins/obs-outputs.so
#install_name_tool -change /tmp/obsdeps/bin/libjansson.4.dylib @executable_path/../Frameworks/libjansson.4.dylib ./OBS.app/Contents/Plugins/obs-outputs.so

# copy sparkle into the app
#hr "Copying Sparkle.framework"
#cp -R ../../sparkle/Sparkle.framework ./RPANStudio.app/Contents/Frameworks/
#install_name_tool -change @rpath/Sparkle.framework/Versions/A/Sparkle @executable_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle ./RPANStudio.app/Contents/MacOS/obs

# Copy Chromium embedded framework to app Frameworks directory
hr "Copying Chromium Embedded Framework.framework"
mkdir -p RPANStudio.app/Contents/Frameworks
cp -R ${CEF_ROOT_DIR}/Release/Chromium\ Embedded\ Framework.framework RPANStudio.app/Contents/Frameworks/

#install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./RPANStudio.app/Contents/Plugins/obs-browser.so
#install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./RPANStudio.app/Contents/Plugins/obs-browser.so
#install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./RPANStudio.app/Contents/Plugins/obs-browser.so

# cp ../CI/install/osx/OBSPublicDSAKey.pem RPANStudio.app/Contents/Resources

# edit plist
#plutil -insert CFBundleVersion -string $GIT_TAG ./RPANStudio.app/Contents/Info.plist
#plutil -insert CFBundleShortVersionString -string $GIT_TAG ./RPANStudio.app/Contents/Info.plist
#plutil -insert OBSFeedsURL -string https://obsproject.com/osx_update/feeds.xml ./RPANStudio.app/Contents/Info.plist
#plutil -insert SUFeedURL -string https://obsproject.com/osx_update/stable/updates.xml ./RPANStudio.app/Contents/Info.plist
#plutil -insert SUPublicDSAKeyFile -string OBSPublicDSAKey.pem ./RPANStudio.app/Contents/Info.plist

rm -rf ../CI/RPAN\ Studio.app
mv RPANStudio.app ../CI/RPAN\ Studio.app

cd ../CI

hr "Fixing app linkage"
./fix_app.py

hr "Signing app"
./codesign.sh ${SIGN_IDENTITY}

hr "Creating DMG"
dmgbuild -s install/osx-rpan/settings.json "RPAN Studio" rpan-studio.dmg

