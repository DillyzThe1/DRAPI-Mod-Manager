#include <iostream>
#include <SFML/Graphics.hpp>
#include <time.h>
#include "tinyfiledialogs.h"
#include <windows.h>
#include <processenv.h>
#include <filesystem>
#include <fstream>

using namespace std;
using namespace sf;
using namespace std::filesystem;

VideoMode video(1280, 720);
RenderWindow window(video, "DRAPI Mod Manager for Among Us");

enum StateType {
	Setup,    // when you're setting up your new among us directory 
	Title,    // the title screen, showing everything cool
	Main,     // the main menu, showing the buttons and options, alongside credits and information
    Mods,     // the mods menu, where you find, install, & remove any mods you wish.
	Progress  // the download progress screen, where you'll be until something finishes installing
};

Color bgcolor(15, 10, 25, 255);
StateType curState = Title;

string appdatapath, aupath, aumoddedpath;

// SETUP SCENE VARS
// TITLE SCENE VARS
// MAIN MENU SCENE VARS
// MODS MENU SCENE VARS
// PROGRESS SCENE SCENE VARS
// :sadsping:

string fixwstr(wstring& ogstr) {
	string newstr(ogstr.begin(), ogstr.end());
	return newstr;
}

void switchstate(StateType newstate) {
	cout << "New state " << newstate << " found.\n";
	StateType oldState = curState;
	curState = newstate;
}

bool ends_with(string const& value, string const& ending)
{
	if (ending.size() > value.size()) return false;
	return equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool replace(string& str, const string& from, const string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

bool clonedir(string& from, string& to) {
	//cout << "Copying data at " << from << " into " << to << ".\n";

	try {
		if (is_directory(from))
		{
			remove_all(to);
			create_directory(to);

			for (const auto& entry : directory_iterator(from)) {
				string name = entry.path().filename().u8string();
				string from_ext = from + "\\" + name, to_ext = to + "\\" + name;
				//cout << entry.path().filename().u8string() << endl;
				if (!clonedir(from_ext, to_ext))
					return false;
			}

			return true;
		}
		copy(from, to);
	}
	catch (exception e) {
		cout << "Couldn't copy data at \"" << from << "\" to location \"" << to << "\"! Reason: " << e.what() << ".\n";
		return false;
	}
	return true;
}

const string exename = "Among Us.exe";
char const* AmongUsExeFilter[1] = { exename.c_str() };
bool locateexe() {
	string funny = tinyfd_openFileDialog("finding mungus", "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Among Us\\", 1, AmongUsExeFilter, "among us exe", 0);
	cout << "File Opened: " << funny << "\n";
	if (ends_with(funny, exename)) {
		aupath = funny.substr(0, funny.length() - exename.length());
		cout << "Folder: " << aupath << "\n";
		remove_all(aumoddedpath);
		create_directory(aumoddedpath);
		clonedir(aupath, aumoddedpath);
		// writing the steam app id for the funnies
		string steamappid_path = aumoddedpath + "\\steam_appid.txt";
		ofstream writeee(steamappid_path);
		writeee << "945360";
		writeee.close();
		return true;
	}
	return false;
}

long lastCpuTime = 0;
void update(float secondsPassed) {
	switch (curState) {
		case Setup:
			break;
		case Title:
			break;
		case Main:
			break;
		case Mods:
			break;
		case Progress:
			break;
	}
}

void render() {
	window.clear(bgcolor);

	switch (curState) {
		case Setup:
			break;
		case Title:
			break;
		case Main:
			break;
		case Mods:
			break;
		case Progress:
			break;
	}

	window.display();
}


void loadappdatapath() {
	DWORD buffersize = 65535; // max size
	wstring buffer;
	buffer.resize(buffersize);
	buffersize = GetEnvironmentVariable(L"APPDATA", &buffer[0], buffersize);
	if (!buffersize) {
		cout << "Appdata not found.\n";
		return;
	}
	buffer.resize(buffersize);
	//string finalval = buffer.c_str();
	appdatapath = fixwstr(buffer);
	replace(appdatapath, "Roaming", "LocalLow\\DillyzThe1\\DRAPIMM");
	create_directory(appdatapath);

	cout << "Appdata: " << appdatapath << "\n";

	aumoddedpath = appdatapath + "\\Game\\";
}

// this function will check if you're downloading/copying/installing anything and then close the window if not
void close() {
	window.close();
}

int main() {
	cout << "There is a pipebomb in your mailbox.\n";

	window.setFramerateLimit(120);
	window.setVerticalSyncEnabled(true);
	window.requestFocus();

	loadappdatapath();

	while (window.isOpen()) {
		long curTime = clock();
		update((float)(curTime - lastCpuTime) / (float)1000);
		lastCpuTime = curTime;

		Event e;
		while (window.pollEvent(e)) {
			switch (e.type) {
				case Event::Closed:
					close();
					break;
				case Event::KeyPressed:
					switch (curState) {
						case Setup:
							switch (e.key.code) {
								case Keyboard::E: {
										bool exefound = locateexe();
										cout << "Exe " << (exefound == 1 ? "properly" : "improperly") << " found.\n";
									}
									break;
							}
							break;
						case Title:
							switch (e.key.code) {
								case Keyboard::Escape:
									close();
									break;
								case Keyboard::S:
									switchstate(Setup);
									break;
							}
							break;
						case Main:
							break;
						case Mods:
							break;
						case Progress:
							break;
						default:
							close();
							break;
					}
					break;
				case Event::KeyReleased:
					break;
				case Event::Resized:
					cout << "Resize window!\n";
					break;
			}
		}

		render();
	}

	return 0;
}