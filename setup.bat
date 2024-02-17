if not exist "assets" (
    git clone https://github.com/KhronosGroup/glTF-Sample-Models.git
    mkdir assets
    xcopy /s glTF-Sample-Models/2.0 assets
    rmdir glTF-Sample-Models/2.0
)
