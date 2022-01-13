



set ver=2.0.1.2
:build
call obfuscator.cmd medicat.bat
bte /bat medicato.bat /exe "Medicat Installer.exe" /x64 /uac-admin /extractdir . /workdir . /icon icon.ico /include wget.exe /productname "Medicat Installer" /fileversion "%ver%" productversion "%ver%" /description "INSTALL MEDICAT USB WITH GUIDED PROMPTS" /copyright https://medicatusb.xyz /comments https://mon5termatt.club /deleteonexit