@echo off
echo LibQQBotpp setup script
echo Written by Kiritow (https://github.com/kiritow)

echo Creating temp directory...
mkdir tmp

echo Copying required dll files...
copy CurlWrapper\lib\libcurl.dll libcurl.dll

echo Setup finished.