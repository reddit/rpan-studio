function Component() {
    if (systemInfo.productType === "windows") {
        installer.installationFinished.connect(
            this,
            Component.prototype.installVCRedist
        );
    }
}

Component.prototype.installVCRedist = function() {
    var dir = installer.value("TargetDir");
    installer.execute(dir + "/redist/vcredist_x86.exe", "/install", "/norestart", "/passive");
    installer.execute(dir + "/redist/vcredist_x64.exe", "/install", "/norestart", "/passive");
}

Component.prototype.createOperations = function() {
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation(
            "CreateShortcut",
            "@TargetDir@/bin/64bit/obs64.exe",
            "@StartMenuDir@/RPAN Studio.lnk",
            "workingDirectory=@TargetDir@/bin/64bit",
            "iconPath=@TargetDir@/bin/64bit/obs64.exe",
            "iconId=0",
            "description=RPAN Studio"
        );

        component.addOperation(
            "CreateShortcut",
            "@TargetDir@/uninstall.exe",
            "@StartMenuDir@/Uninstall.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/uninstall.exe",
            "iconId=0",
            "description=Uninstall RPAN Studio"
        )
    }
}
