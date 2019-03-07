#include "Save.hpp"

#include <iostream>
#include <shobjidl.h>
#include <string>

std::string BasicFileOpen()
{
	if(CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) < 0)
		return "";

	// Create the FileOpenDialog object.
	IFileOpenDialog *pFileOpen;
	if (CoCreateInstance(CLSID_FileOpenDialog,
		0, CLSCTX_ALL, IID_IFileOpenDialog,
		reinterpret_cast<void**>(&pFileOpen)) < 0)
		return "";

	// Show the Open dialog box.
	if (pFileOpen->Show(0) < 0)
		return "";
	
	// Get the file name from the dialog box.
	IShellItem *pItem;
	if (pFileOpen->GetResult(&pItem) < 0)
		return "";
	
	wchar_t * pszFilePath;
	
	// Display the file name to the user.
	if (pItem->GetDisplayName(SIGDN_FILESYSPATH, (LPWSTR *)&pszFilePath) < 0)
		return "";

	std::wstring ws(pszFilePath);
	std::string str(ws.begin(), ws.end());

	CoTaskMemFree(pszFilePath);
	pItem->Release();
	pFileOpen->Release();
	CoUninitialize();

	return str;
}


int main(int argc, char **argv) {
	Save save;
	std::string fileLocation;
	if (argc <= 1) {
		fileLocation = BasicFileOpen();
	} else {
		//"./../slot0.sav"
		fileLocation = std::string(argv[1]);
	}

	std::cout << "Opening file: " << fileLocation << std::endl;

	save.parse(fileLocation.c_str());
	return 0;
}