#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
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
RenderWindow window(video, "DRAPI Mod Manager for Among Us", Style::Default, ContextSettings::ContextSettings(0, 0, 4, 1, 1, 0, false));
LPCWSTR titlebutgoofy = L"DRAPI Mod Manager for Among Us";

enum StateType {
	None,     // default so i can call things
	Main,     // the main menu, showing the buttons and options, alongside credits and information
    Mods,     // the mods menu, where you find, install, & remove any mods you wish.
	Progress  // the download progress screen, where you'll be until something finishes installing
};

Color color_bg(15, 10, 25, 255), color_white(255, 255, 255, 255), color_selected(0, 255, 0, 255), color_deselected(128, 128, 128, 255);
StateType curState = None;

string launcherdataURL = "https://cdn.discordapp.com/attachments/849292573230104576/1060836092003225681/launcher_latest.json";
string announcmentdataURL = "https://cdn.discordapp.com/attachments/849292573230104576/1060820994429812736/announcement.json";

string appdatapath, aupath, aumoddedpath, launcherdatapath, announcmentsdatapath, bepinexzippath;
json launcherjson, announcementjson;

path userdatapath;

const int launcherversion = 0;
json userdata = {
	{"last_announcement", -1},
	{"last_bepinex", -1},
	{"last_auversion", "1970.1.1"},
	{"setup_properly", false},
	{"mods_installed", {
		{
			{"name", "DillyzRoleApi"},
			{"last_version", -1},
			{"last_versionname", "v0.0.0-dev"}
		}
	}}
};

void saveuserdata() {
	ofstream writeee(userdatapath.string());
	writeee << userdata.dump(4);
	writeee.close();
}

FloatRect vz(0, 0, 1280, 720);
Vector2i getMousePos() {
	if (!window.hasFocus())
		return Vector2i(-100, -100);
	Vector2i mp = Mouse::getPosition();
	Vector2i wp = window.getPosition();
	Vector2u ws = window.getSize();
	Vector2i smp = Vector2i(mp.x - wp.x - 8, mp.y - wp.y - 30);
	Vector2f perc = Vector2f((double)smp.x/(double)ws.x, (double)smp.y/(double)ws.y);
	Vector2i mpiw = Vector2i(vz.width * perc.x, vz.height * perc.y);
	return mpiw;
}

sf::SoundBuffer sb_hover, sb_select, sb_complete, sb_progress, sb_appear, sb_disappear;
sf::Sound sfx_hover, sfx_select, sfx_complete, sfx_progress, sfx_appear, sfx_disappear;

// SETUP SCENE VARS
// TITLE SCENE VARS
// MAIN MENU SCENE VARS
bool mm_setup = false;
Texture mm_buttontex, mm_logotex;
Sprite mm_logo;
Sprite mm_button_launch, mm_button_mods, mm_button_reinstall, mm_button_howtomod;

bool launchdisabled = false, modsdisabled = false, reinstalldisabled = false, howtodisabled = false;
bool prevhov_launch = false, prevhov_mods = false, prevhov_reinstall = false, prevhov_howtomod = false;
// --Sprite mm_minibutton_announcements, mm_minibutton_settings, mm_minibutton_innersloth, mm_minibutton_refresh, mm_minibutton_discord;
// MODS MENU SCENE VARS
// PROGRESS SCENE SCENE VARS
// :sadsping:



string fixwstr(wstring& ogstr) {
	string newstr(ogstr.begin(), ogstr.end());
	return newstr;
}

int width = 1280, height = 720;
void reposscene() {
	float centx = width / 2, centy = height / 2;
	switch (curState) {
		case Main: {
				mm_logo.setPosition(Vector2f(centx - mm_logotex.getSize().x / 2, centy - mm_logotex.getSize().y - 25));

				int offset = -(192/2);
				mm_button_launch.setPosition(Vector2f(centx - 100 + offset, centy));
				mm_button_mods.setPosition(Vector2f(centx + 100 + offset, centy));
				mm_button_reinstall.setPosition(Vector2f(centx - 100 + offset, centy + 90));
				mm_button_howtomod.setPosition(Vector2f(centx + 100 + offset, centy + 90));
			}
			break;
		case Mods:
			break;
		case Progress:
			break;
	}
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

bool installingnow = false;
const string exename = "Among Us.exe";
char const* AmongUsExeFilter[1] = { exename.c_str() };
bool locateexe() {
	userdata["setup_properly"] = false;
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

		// modify user data
		userdata["last_bepinex"] = launcherjson["bepinex_vers"];
		userdata["last_auversion"] = launcherjson["auvers"];
		userdata["setup_properly"] = true;
		saveuserdata();
		return true;
	}
	return false;
}

bool hoveringSprite(Vector2i& mp, Sprite& spr, int width, int height) {
	Vector2i coolstuffs = Vector2i(mp.x - spr.getPosition().x, mp.y - spr.getPosition().y);
	bool hovering = !(coolstuffs.x < 0 || coolstuffs.y < 0 || coolstuffs.x > width || coolstuffs.y > height);
	spr.setColor(hovering ? color_selected : color_white);
	return hovering;
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
	userdatapath = path::path(appdatapath + "\\userdata.json");
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
		//cout << "mod " << i << endl;
		string funnyname = launcherjson["mods"][i]["name"];
		string banner = launcherjson["mods"][i]["banner"];
		string top10logic = bannerspath + "\\" + funnyname + ".png";

		// don't redownload a pre-existing banner
		if (exists(top10logic))
			continue;

		//cout << "mdddod " << funnyname << " - " << banner << " - " << top10logic << endl;
		if (starts_with(banner, "https://"))
			download(banner, top10logic);
		//else
		//	cout << banner << " doesn't start with https://\n";
	}

	return true;
}

json buttondata;
void scenesetup_mainmenu() {
	if (mm_setup)
		return;
	mm_setup = true;

	ifstream buttondata_stream("content/data/buttons.json");
	buttondata = json::parse(buttondata_stream);
	buttondata_stream.close();

	int s0bx = buttondata["size0_bound_x"], s0by = buttondata["size0_bound_y"];
	int s1bx = buttondata["size1_bound_x"], s1by = buttondata["size1_bound_y"];
	int s2bx = buttondata["size2_bound_x"], s2by = buttondata["size2_bound_y"];

	mm_logotex.loadFromFile("content/graphics/logo.png");
	mm_buttontex.loadFromFile("content/graphics/buttons.png");

	mm_logo.setTexture(mm_logotex);

	mm_button_launch.setTexture(mm_buttontex);
	mm_button_launch.setTextureRect(IntRect(200, 0, s0bx, s0by));

	mm_button_mods.setTexture(mm_buttontex);
	mm_button_mods.setTextureRect(IntRect(400, 0, s0bx, s0by));

	mm_button_reinstall.setTexture(mm_buttontex);
	mm_button_reinstall.setTextureRect(IntRect(200, 88, s1bx, s1by));

	mm_button_howtomod.setTexture(mm_buttontex);
	mm_button_howtomod.setTextureRect(IntRect(400, 88, s1bx, s1by));



	mm_logo.setPosition(Vector2f(400, 0));

	mm_button_launch.setPosition(Vector2f(0, 0));
	mm_button_mods.setPosition(Vector2f(200, 0));
	mm_button_reinstall.setPosition(Vector2f(0, 90));
	mm_button_howtomod.setPosition(Vector2f(200, 90));

	reposscene();
}

void switchstate(StateType newstate) {
	cout << "New state " << newstate << " found.\n";
	StateType oldState = curState;
	curState = newstate;

	switch (newstate) {
		case Main:
			scenesetup_mainmenu();
			break;
	}

	reposscene();
}

string wikilink = "https://github.com/DillyzThe1/DillyzRoleApi-Rewritten/wiki";
bool prevpressed = false;
long lastCpuTime = 0;
void update(float secondsPassed) {
	Vector2i mp = getMousePos();
	bool pressing = Mouse::isButtonPressed(Mouse::Left);
	bool justpressed = prevpressed != pressing && pressing;
	switch (curState) {
		case Main: {
			bool hov_launch = !launchdisabled && hoveringSprite(mp, mm_button_launch, 192, 80);
			if (launchdisabled || installingnow)
				mm_button_launch.setColor(color_deselected);

			bool hov_mods = !modsdisabled && hoveringSprite(mp, mm_button_mods, 192, 80);
			if (modsdisabled || installingnow)
				mm_button_mods.setColor(color_deselected);

			bool hov_reinstall = !reinstalldisabled && hoveringSprite(mp, mm_button_reinstall, 192, 50);
			if (reinstalldisabled || installingnow)
				mm_button_reinstall.setColor(color_deselected);

			bool hov_howtomod = !howtodisabled && hoveringSprite(mp, mm_button_howtomod, 192, 50);
			if (howtodisabled || installingnow)
				mm_button_howtomod.setColor(color_deselected);

			if ((hov_launch != prevhov_launch && hov_launch)
				|| (hov_mods != prevhov_mods && hov_mods)
				|| (hov_reinstall != prevhov_reinstall && hov_reinstall)
				|| (hov_howtomod != prevhov_howtomod && hov_howtomod))
				sfx_hover.play();

			if (justpressed && !installingnow) {
				if (hov_launch) {
					sfx_select.play();
					string exepath = aumoddedpath + exename;
					ShellExecuteA(NULL, "open", exepath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					return;
				}
				if (hov_mods) {
					sfx_select.play();
					switchstate(Mods);
					return;
				}
				if (hov_reinstall) {
					sfx_select.play();
					sfx_appear.play();
					try {
						installingnow = true;
						bool exefound = locateexe();
						cout << "Exe " << (exefound == 1 ? "properly" : "improperly") << " found.\n";
						sfx_complete.play();
					}
					catch (exception e) {
						cout << "Could not properly setup Among Us directory! Reason: " << e.what() << ".\n";
						sfx_progress.play();
					}
					launchdisabled = modsdisabled = howtodisabled = !userdata["setup_properly"];
					installingnow = false;
					sfx_disappear.play();
					return;
				}
				if (hov_howtomod) {
					sfx_select.play();
					//ShellExecuteA(NULL, "open", appdatapath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					ShellExecuteA(NULL, "open", wikilink.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					return;
				}
			}

			prevhov_launch = hov_launch, prevhov_mods = hov_mods, prevhov_reinstall = hov_reinstall, prevhov_howtomod = hov_howtomod;
		}
			break;
		case Mods:
			break;
		case Progress:
			break;
	}
	prevpressed = pressing;
}

void render() {
	window.clear(color_bg);

	switch (curState) {
		case Main:
			if (!mm_setup)
				return;

			window.draw(mm_logo);

			window.draw(mm_button_launch);
			window.draw(mm_button_mods);
			window.draw(mm_button_reinstall);
			window.draw(mm_button_howtomod);
			break;
		case Mods:
			break;
		case Progress:
			break;
	}

	window.display();
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

	// make defaults just incase
	if (!exists(userdatapath))
		saveuserdata();
	else {
		ifstream userdata_stream(userdatapath.string());
		userdata = json::parse(userdata_stream);
		userdata_stream.close();
	}

	launchdisabled = modsdisabled = howtodisabled = !userdata["setup_properly"];

	// load sounds
	sb_hover.loadFromFile("content/audio/UI_Hover.ogg");
	sfx_hover.setBuffer(sb_hover);
	sfx_hover.setVolume(50);

	sb_select.loadFromFile("content/audio/UI_Select.ogg");
	sfx_select.setBuffer(sb_select);
	sfx_select.setVolume(50);

	sb_complete.loadFromFile("content/audio/task_Complete.ogg");
	sfx_complete.setBuffer(sb_complete);
	sfx_complete.setVolume(50);

	sb_progress.loadFromFile("content/audio/task_Inprogress.ogg");
	sfx_progress.setBuffer(sb_progress);
	sfx_progress.setVolume(50);

	sb_appear.loadFromFile("content/audio/Panel_GenericAppear.ogg");
	sfx_appear.setBuffer(sb_appear);
	sfx_appear.setVolume(50);

	sb_disappear.loadFromFile("content/audio/Panel_GenericDisappear.ogg");
	sfx_disappear.setBuffer(sb_disappear);
	sfx_disappear.setVolume(50);
	//

	MessageBox(NULL, L"This program is unfinished, but hello anyway!\nBinds:\n- S to switch to Setup.\n- E to locate EXE.\n- A to download files.\n- O to open your LocalLow folder.\n\nYou only need to hit S once & other binds require an S press.\nOk bye!", titlebutgoofy, MB_ICONINFORMATION);

	switchstate(Main);
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
						case Main:
							switch (e.key.code) {
								case Keyboard::Escape:
									close();
									break;
								case Keyboard::M:
									switchstate(Mods);
									break;
							}
							break;
						case Mods:
							switch (e.key.code) {
								case Keyboard::Escape:
									switchstate(Main);
									break;
							}
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
					bool aspect_height = e.size.width > e.size.height;
					double aspect = aspect_height ? ((double)e.size.width / e.size.height) : ((double)e.size.height / e.size.width);
					cout << "Aspect ratio: " << aspect << " " << aspect_height << endl;
					vz = FloatRect(0, 0, aspect_height ? (720 * aspect) : 1280, aspect_height ? 720 : (1280 * aspect));
					window.setView(View(vz));
					width = vz.width;
					height = vz.height;
					reposscene();
					break;
			}
		}

		render();
	}

	return 0;
}