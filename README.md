# EvolandSaveEditor
This program translates the ".sav" files for the game "Evoland" into a more easily viewable/editable ".json", and can translate compatible ".json" files back into ".sav" files.

## Installing
[unzip this file](https://raw.githubusercontent.com/wabuilderman/EvolandSaveEditor/master/Release.zip), and click "Release", then either **"Win32"** or **"x64"**, depending on your needs (If you aren't sure, click **"Win32"**.) Inside that folder, you will see a file called "EvolandSaveEditor.exe".

## Decoding a ".sav" file
### GUI
Double-click on the exe. A popup should ask you for a folder. Select the ".sav" file you wish to decode. The program will then create a ".json" file with the same name in that same directory (overriding the file if it is already there).
### Example of Commandline Syntax
C:\EvolandSaveEditor\Release\Win32> EvolandSaveEditor.exe C:/mySavefile.sav

## Encoding a ".sav" file
### GUI
Double-click on the exe. A popup should ask you for a folder. Select the ".json" file you wish to decode. The program will then create a ".sav" file with the same name in that same directory (overriding the file if it is already there).
### Example of Commandline Syntax
C:\EvolandSaveEditor\Release\Win32> EvolandSaveEditor.exe C:/mySavefile.json
