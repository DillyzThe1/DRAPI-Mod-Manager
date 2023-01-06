#include <iostream>
#include <SFML/Graphics.hpp>
#include <time.h>
#include "tinyfiledialogs.h"
#include <windows.h>
#include <processenv.h>
#include <filesystem>
#include <fstream>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#include <wininet.h>
#pragma comment(lib,"Wininet.lib")
#include "json.hpp"
#include "miniz.h"

using namespace std;
using namespace sf;
using namespace std::filesystem;
using json = nlohmann::json;

VideoMode video(1280, 720);
RenderWindow window(video, "DRAPI Mod Manager for Among Us");
LPCWSTR titlebutgoofy = L"DRAPI Mod Manager for Among Us";

enum StateType {
	Setup,    // when you're setting up your new among us directory 
	Title,    // the title screen, showing everything cool
	Main,     // the main menu, showing the buttons and options, alongside credits and information
    Mods,     // the mods menu, where you find, install, & remove any mods you wish.
	Progress  // the download progress screen, where you'll be until something finishes installing
};

Color bgcolor(15, 10, 25, 255);
StateType curState = Title;

string launcherdataURL = "https://cdn.discordapp.com/attachments/849292573230104576/1060836092003225681/launcher_latest.json";
string announcmentdataURL = "https://cdn.discordapp.com/attachments/849292573230104576/1060820994429812736/announcement.json";

string appdatapath, aupath, aumoddedpath, launcherdatapath, announcmentsdatapath, bepinexzippath;
json launcherjson, announcementjson;

/*struct ModDependencyData {
	string name;
	int version;
	string versionname;
};

struct ModData {
	string name;
	string file;
	string banner;
	string bannerhash;
	int version;
	string versionname;
	string description;
	string author;
	list<ModDependencyData> dependencies;
};*/

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

bool starts_with(string const& value, string const& start)
{
	if (start.size() > value.size()) return false;
	return value.substr(0, start.size()) == start;
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

bool download(string& link, string& file) {
	if (!starts_with(link, "https:")) // prevents crashes and insecure links
		return false;
	URLDownloadToFile(NULL, wstring(link.begin(), link.end()).c_str(), wstring(file.begin(), file.end()).c_str(), BINDF_GETNEWESTVERSION, NULL);
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
		
		// download bepinex
		string bepinexlink = launcherjson["bepinex"];
		download(bepinexlink, bepinexzippath);

		// extracting bepinex
		mz_zip_archive ziparchive{};
		path dp{bepinexzippath};
		if (!mz_zip_reader_init_file(&ziparchive, dp.string().c_str(), 0))
			return false; // failed to initiate the zip
		const int fileamt = (int)mz_zip_reader_get_num_files(&ziparchive);
		cout << "files found (" << fileamt << ")\n";
		if (fileamt == 0)
			return false; // failed to find any contents
		mz_zip_archive_file_stat fs;
		if (!mz_zip_reader_file_stat(&ziparchive, 0, &fs)) {
			mz_zip_reader_end(&ziparchive);
			return false;
		}
		cout << "in the " << aumoddedpath << endl;
		for (int i = 0; i < fileamt; i++) {
			mz_zip_reader_file_stat(&ziparchive, i, &fs);
			string newpath = aumoddedpath + fs.m_filename;
			path dir(newpath);
			//cout << "check the " << newpath << " " << dir.parent_path().string() << endl;
			string funnydir = dir.parent_path().string();
			string top10awesome = funnydir.substr(aumoddedpath.length() - 1, funnydir.length());
			if (top10awesome.length() != 0)
				create_directories(dir.parent_path());
			path fileout(dir.parent_path().generic_string() + "\\" + dir.filename().string());
			if (!mz_zip_reader_extract_to_file(&ziparchive, i, fileout.string().c_str(), 0))
			{
				mz_zip_reader_end(&ziparchive);
				return false; // epic file ending
			}
		}
		if (!mz_zip_reader_end(&ziparchive))
			return false; // epic zip fail
		// for mods
		create_directories(aumoddedpath + "\\BepInEx\\plugins");

		// now let's trash bepinex.zip
		remove(bepinexzippath);
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
	string appfolder = "DRAPIMM";
	replace(appdatapath, "Roaming", "LocalLow\\DillyzThe1\\" + appfolder);
	create_directory(appdatapath.substr(0, appdatapath.length() - appfolder.length()));
	create_directory(appdatapath);

	cout << "Appdata: " << appdatapath << "\n";

	aumoddedpath = appdatapath + "\\Game\\";
	launcherdatapath = appdatapath + "\\launcher_latest.json";
	announcmentsdatapath = appdatapath + "\\announcement.json";
	bepinexzippath = appdatapath + "\\bepinex.zip";
}


bool downloaddata() {
	//cout << "We should download \"" << launcherdatalink << "\" to file \"" << ldp_wstr << "\".\n";
	bool bConnect = InternetCheckConnection(L"https://www.google.com/", FLAG_ICC_FORCE_CONNECTION, 0);
	if (!bConnect)
		return false;
	download(launcherdataURL, launcherdatapath);
	download(announcmentdataURL, announcmentsdatapath);
	
	ifstream launcherjson_stream(launcherdatapath);
	launcherjson = json::parse(launcherjson_stream);
	launcherjson_stream.close();

	ifstream announcmentjson_stream(announcmentsdatapath);
	announcementjson = json::parse(announcmentjson_stream);
	announcmentjson_stream.close();

	// download all banners
	string bannerspath = appdatapath + "\\banners";
	create_directory(bannerspath);

	cout << "mods list has " << launcherjson["mods"].size() << " mods\n";

	for (int i = 0; i < launcherjson["mods"].size(); i++) {
		cout << "mod " << i << endl;
		string funnyname = launcherjson["mods"][i]["name"];
		string banner = launcherjson["mods"][i]["banner"];
		string top10logic = bannerspath + "\\" + funnyname + ".png";
		cout << "mdddod " << funnyname << " - " << banner << " - " << top10logic << endl;
		if (starts_with(banner, "https://"))
			download(banner, top10logic);
		else
			cout << banner << " doesn't start with https://\n";
	}

	return true;
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
	if (!downloaddata()) {
		window.close();
		MessageBox(NULL, L"Internet access not found!", titlebutgoofy, MB_ICONERROR);
		return 0;
	}

	MessageBox(NULL, L"This program is unfinished, but hello anyway!\nBinds:\n- S to switch to Setup.\n- E to locate EXE.\n- A to download files.\n- O to open your LocalLow folder.\n\nYou only need to hit S once & other binds require an S press.\nOk bye!", titlebutgoofy, MB_ICONINFORMATION);

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
										try {
											bool exefound = locateexe();
											cout << "Exe " << (exefound == 1 ? "properly" : "improperly") << " found.\n";
										}
										catch (exception e) {
											cout << "Could not properly setup Among Us directory! Reason: " << e.what() << ".\n";
										}
									}
									break;
								case Keyboard::A:
									downloaddata();
									break;
								case Keyboard::O:
									ShellExecuteA(NULL, "open", appdatapath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
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