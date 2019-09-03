git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
git pull
call bootstrap-vcpkg.bat
.\vcpkg.exe update
.\vcpkg.exe install tesseract opencv boost-program-options