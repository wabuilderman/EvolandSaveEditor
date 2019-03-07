# EvolandSaveEditor
For the time being (contrary to the project title), this program only translates the ".sav" files for the game "Evoland" into a more easily viewable/editable ".json"

## Installing
[unzip this file](https://raw.githubusercontent.com/user/repository/branch/filename), and click "Release", then either **"Win32"** or **"x64"**, depending on your needs (If you aren't sure, click **"Win32"**.) Inside that folder, you will see a file called "EvolandSaveEditor.exe".

## Decoding a ".sav" file
### GUI
Double-click on the exe. A popup should ask you for a folder. Select the ".sav" file you wish to decode. The program will then create a ".json" file with the same name in that same directory (overriding the file if it is already there).
### Example of Commandline Syntax
C:\EvolandSaveEditor\Release\Win32> EvolandSaveEditor.exe C:/mySavefile.sav
