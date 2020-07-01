# Exit if something fails
set -e

if [ -z "${BUILD_TYPE}" ] ; then
    BUILD_TYPE=RelWithDebInfo
fi

rm -rf ./RPANStudio.app

mkdir -p RPANStudio.app/Contents/{MacOS,PlugIns,Resources,Frameworks}

cp -R rundir/${BUILD_TYPE}/bin/* ./RPANStudio.app/Contents/MacOS
cp -R rundir/${BUILD_TYPE}/data ./RPANStudio.app/Contents/Resources
cp ../CI/install/osx-rpan/rpan-studio.icns ./RPANStudio.app/Contents/Resources
cp -R rundir/${BUILD_TYPE}/obs-plugins/* ./RPANStudio.app/Contents/PlugIns
cp ../CI/install/osx-rpan/Info.plist ./RPANStudio.app/Contents

#../CI/install/osx-rpan/dylibBundler -b -cd -d ./RPANStudio.app/Contents/Frameworks -p @executable_path/../Frameworks/ \
#-s ./RPANStudio.app/Contents/MacOS \
#-s /usr/local/opt/mbedtls/lib/ \
#-x ./RPANStudio.app/Contents/PlugIns/coreaudio-encoder.so \
#-x ./RPANStudio.app/Contents/PlugIns/decklink-ouput-ui.so \
#-x ./RPANStudio.app/Contents/PlugIns/frontend-tools.so \
#-x ./RPANStudio.app/Contents/PlugIns/image-source.so \
#-x ./RPANStudio.app/Contents/PlugIns/mac-avcapture.so \
#-x ./RPANStudio.app/Contents/PlugIns/mac-capture.so \
#-x ./RPANStudio.app/Contents/PlugIns/mac-decklink.so \
#-x ./RPANStudio.app/Contents/PlugIns/mac-syphon.so \
#-x ./RPANStudio.app/Contents/PlugIns/mac-vth264.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-browser.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-browser-page \
#-x ./RPANStudio.app/Contents/PlugIns/obs-ffmpeg.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-filters.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-outputs.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-transitions.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-vst.so \
#-x ./RPANStudio.app/Contents/PlugIns/obs-x264.so \
#-x ./RPANStudio.app/Contents/PlugIns/rtmp-services.so \
#-x ./RPANStudio.app/Contents/PlugIns/text-freetype2.so
#-x ./RPANStudio.app/Contents/MacOS/obs \
#-x ./RPANStudio.app/Contents/MacOS/obs-ffmpeg-mux \
#-x ./RPANStudio.app/Contents/MacOS/obslua.so \
#-x ./RPANStudio.app/Contents/MacOS/_obspython.so
# -x ./RPANStudio.app/Contents/PlugIns/obs-libfdk.so
# -x ./RPANStudio.app/Contents/PlugIns/linux-jack.so \

#/usr/local/Cellar/qt/5.14.1/bin/macdeployqt ./RPANStudio.app

mv ./RPANStudio.app/Contents/MacOS/libobs-opengl.so ./RPANStudio.app/Contents/Frameworks/

#rm -f -r ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework

# put qt network in here becasuse streamdeck uses it
#cp -R /usr/local/opt/qt/lib/QtNetwork.framework ./RPANStudio.app/Contents/Frameworks
#chmod -R +w ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework
#rm -r ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework/Headers
#rm -r ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework/Versions/5/Headers/
#chmod 644 ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework/Versions/5/Resources/Info.plist
#install_name_tool -id @executable_path/../Frameworks/QtNetwork.framework/Versions/5/QtNetwork ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork
#install_name_tool -change /usr/local/Cellar/qt/5.14.1/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./RPANStudio.app/Contents/Frameworks/QtNetwork.framework/Versions/5/QtNetwork


# decklink ui qt
#install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./RPANStudio.app/Contents/PlugIns/decklink-ouput-ui.so
#install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./RPANStudio.app/Contents/PlugIns/decklink-ouput-ui.so
#install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./RPANStudio.app/Contents/PlugIns/decklink-ouput-ui.so

# frontend tools qt
#install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./RPANStudio.app/Contents/PlugIns/frontend-tools.so
#install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./RPANStudio.app/Contents/PlugIns/frontend-tools.so
#install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./RPANStudio.app/Contents/PlugIns/frontend-tools.so

# vst qt
#install_name_tool -change /usr/local/opt/qt/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./RPANStudio.app/Contents/PlugIns/obs-vst.so
#install_name_tool -change /usr/local/opt/qt/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./RPANStudio.app/Contents/PlugIns/obs-vst.so
#install_name_tool -change /usr/local/opt/qt/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./RPANStudio.app/Contents/PlugIns/obs-vst.so
#install_name_tool -change /usr/local/opt/qt/lib/QtMacExtras.framework/Versions/5/QtMacExtras @executable_path/../Frameworks/QtMacExtras.framework/Versions/5/QtMacExtras ./RPANStudio.app/Contents/PlugIns/obs-vst.so
