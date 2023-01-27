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

string launcherdataURL = "https://raw.githubusercontent.com/DillyzThe1/DRAPI-Mod-Manager/main/data/launcher_latest.json";
string announcmentdataURL = "https://raw.githubusercontent.com/DillyzThe1/DRAPI-Mod-Manager/main/data/announcement.json";

string appdatapath, aupath, aumoddedpath, launcherdatapath, announcmentsdatapath, bepinexzippath;
json launcherjson, announcementjson;

path userdatapath;

const int launcherversion = 74;
const string launcherversionname = "2023.1.7";
json userdata = {
	{"last_announcement", -1},
	{"last_bepinex", -1},
	{"last_auversion", "1970.1.1"},
	{"setup_properly", false},
	{"mods_installed", {
		{
			{"name", "DillyzRoleApi"},
			{"last_version", -1},
			{"last_versionname", "v0.0.0-dev"},
			{"active", true}
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

Font font_ui, font_buttons;
Text verstext;


float cooldown = 1.5;
int width = 1280, height = 720;

// MAIN MENU SCENE VARS
bool mm_setup = false;
Texture mm_buttontex, mm_logotex;
Sprite mm_logo;
Sprite mm_button_launch, mm_button_mods, mm_button_reinstall, mm_button_howtomod;

enum MiniButtonID {
	Announcements, 
	Settings, 
	Innersloth, 
	Refresh, 
	Discord
};

const MiniButtonID mbids[] = {Announcements, Settings, Innersloth, Refresh, Discord};
const int mbrect_x[] =       {0,             75,       150,        225,     300};
const int mbcount = (sizeof(mbids) / sizeof(*mbids));
bool mbprevhold[mbcount];
Sprite minibuttons[mbcount];

bool launchdisabled = false, modsdisabled = false, reinstalldisabled = false, howtodisabled = false;
bool prevhov_launch = false, prevhov_mods = false, prevhov_reinstall = false, prevhov_howtomod = false;
string innerslothlink = "https://www.innersloth.com/", discordlink = "https://discord.gg/49NFTwcYgZ";
// MODS MENU SCENE VARS
Texture modmenu_buttontex, modmenu_modbartex;
bool modmenu_setup = false;
Sprite modbar;
Sprite modmenu_about, modmenu_install, modmenu_issues, modmenu_sourcecode;
Sprite modmenu_left, modmenu_right;
bool prevhov_about = false, prevhov_install = false, prevhov_issues = false, prevhov_sourcecode = false, prevhov_left = false, prevhov_right = false;

string versionstr = "v" + launcherversionname + " (build num " + to_string(launcherversion) + ")";

IntRect installrect(0, 0, 192, 80), updaterect(200, 0, 192, 80), uninstallrect(400, 0, 192, 80);

struct ModDependencyData {
	string name;
	int version;
	string versionname;

	ModDependencyData() {
		this->name = "blank";
	}

	ModDependencyData(string _name, int _version, string _versionname) {
		this->name = _name;
		this->version = _version;
		this->versionname = _versionname;
	}
};

ModDependencyData dependencyfrom(string _name, int _version, string _versionname) {
	ModDependencyData funny(_name, _version, _versionname);
	return funny;
}

struct ModData {
	string name;
	string file;
	string banner;
	string bannerhash;
	int version;
	string versionname;
	string description;
	string author;

	string source;

	int dependencycount = 0;
	ModDependencyData dependencies[10];

	ModData() {
		this->name = "blank";
	}

	ModData(string _name, string _file, string _banner, string _bh, int _version, string _vn, string _description, string _author, string _source) {
		this->name = _name;
		this->file = _file;
		this->banner = _banner;
		this->bannerhash = _bh;
		this->version = _version;
		this->versionname = _vn;
		this->description = _description;
		this->author = _author;
		this->source = _source;
	}

	void AddDependency(ModDependencyData d) {
		dependencies[dependencycount] = d;
		dependencycount++;
	}
};

int modsactive = 0;
ModData mods[100];
Texture modbanners[100];
Sprite curmodbanner;

int curmod = 0;

Text modnametext;

int availableInstallerAction = 0; // 0 to install, 1 to update, 2 to uninstall
// PROGRESS SCENE SCENE VARS
// :sadsping:

void modmenu_move(int amt) {
	if (curmod + amt < 0 || curmod + amt >= modsactive)
		return;

	if (amt != 0) {
		if (cooldown >= 0)
			return;

		cooldown = 0.175;
		curmod += amt;
		sfx_select.play();
		if (curmod < 0)
			curmod = 0;
		else if (curmod >= modsactive)
			curmod = modsactive - 1;
	}

	modnametext.setString(mods[curmod].name);
	modnametext.setPosition((width / 2) - (modnametext.getLocalBounds().width / 2), modbar.getPosition().y + 50 - 36);
	curmodbanner.setTexture(modbanners[curmod]);

	//cout << "getting there\n";
	availableInstallerAction = 0;
	modmenu_install.setTextureRect(installrect);
	for (int i = 0; i < userdata["mods_installed"].size(); i++) {
		if (!userdata["mods_installed"][i]["active"])
			continue;

		string name = userdata["mods_installed"][i]["name"];
		int l_version = userdata["mods_installed"][i]["last_version"];
		string l_versionname = userdata["mods_installed"][i]["last_versionname"];

		//cout << name << " update #" << l_version << " vs " << mods[curmod].name << " update #" << mods[curmod].version << endl;
		if (name != mods[curmod].name)
			continue;

		if (l_version < mods[curmod].version) {
			availableInstallerAction = 1;
			modmenu_install.setTextureRect(updaterect);
			return;
		}

		availableInstallerAction = 2;
		modmenu_install.setTextureRect(uninstallrect);
		return;
	}
}

string fixwstr(wstring& ogstr) {
	string newstr(ogstr.begin(), ogstr.end());
	return newstr;
}

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

				for (int i = 0; i < mbcount; i++)
					minibuttons[i].setPosition(centx - ((float)67 / (float)2) + (((float)i*2 - (float)mbcount) + 1) * 50, centy + 160);
			}
			break;
		case Mods: {
			modmenu_move(0);

			curmodbanner.setPosition(centx - 1280/2, centy - 720/2);

			modmenu_about.setPosition(centx - 200 - 100, height - 130);
			modmenu_install.setPosition(centx - 100, height - 160);
			modmenu_issues.setPosition(centx + 200 - 100, height - 130);

			modmenu_sourcecode.setPosition(centx - 100, height - 75);


			modmenu_left.setPosition(50, height - 45 - 67);
			modmenu_right.setPosition(width - 117, height - 45 - 67);

			modbar.setScale(Vector2f(width >= 2000 ? ((float)width / (float)2000) : 1, 1));
			modbar.setPosition(0, centy + 65);

			modnametext.setPosition((width / 2) - (modnametext.getLocalBounds().width / 2), modbar.getPosition().y + 50 - 36);
		}
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
	char *funny_old = tinyfd_openFileDialog("finding mungus", "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Among Us\\", 1, AmongUsExeFilter, "among us exe", 0);

	sfx_disappear.play();
	//cout << "xd\n";
	if (((funny_old != NULL) && (funny_old[0] == '\0')) || funny_old == NULL)
		return false; // it was skipped

	for (int i = 0; i < userdata["mods_installed"].size(); i++)
		userdata["mods_installed"][i]["active"] = false;

	// convert for funny
	string funny = funny_old;

	userdata["setup_properly"] = false;
	saveuserdata();
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

void showannouncement() {
	string title = (string)announcementjson["title"] + " (by " + (string)announcementjson["author"] + " on " + (string)announcementjson["date"] + ")";
	string pranked = announcementjson["text"];
	userdata["last_announcement"] = announcementjson["version"];
	saveuserdata();
	sfx_appear.play();
	MessageBox(NULL, wstring(pranked.begin(), pranked.end()).c_str(), wstring(title.begin(), title.end()).c_str(), MB_ICONINFORMATION);
	sfx_disappear.play();
}

bool downloaddata() {
	//cout << "We should download \"" << launcherdatalink << "\" to file \"" << ldp_wstr << "\".\n";
	bool bConnect = InternetCheckConnection(L"https://www.google.com/", FLAG_ICC_FORCE_CONNECTION, 0);
	if (!bConnect)
		return false;
	remove(launcherdatapath);
	remove(announcmentsdatapath);
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

	modsactive = 0;

	cout << "mods list has " << launcherjson["mods"].size() << " mods\n";

	for (int i = 0; i < launcherjson["mods"].size(); i++) {
		//cout << "mod " << i << endl;
		string funnyname = launcherjson["mods"][i]["name"];
		string banner = launcherjson["mods"][i]["banner"];
		string top10logic = bannerspath + "\\" + funnyname + ".png";

		modsactive++;

		ModData mod(funnyname, launcherjson["mods"][i]["file"], banner, launcherjson["mods"][i]["bannerhash"], launcherjson["mods"][i]["version"],
			launcherjson["mods"][i]["versionname"], launcherjson["mods"][i]["description"], 
			launcherjson["mods"][i]["author"], launcherjson["mods"][i]["source"]);
		mods[i] = mod;

		for (int o = 0; o < launcherjson["mods"][i]["dependencies"].size(); o++) {
			ModDependencyData dependency(launcherjson["mods"][i]["dependencies"][0]["name"],
				launcherjson["mods"][i]["dependencies"][0]["version"], launcherjson["mods"][i]["dependencies"][0]["versionname"]);
			mods[i].AddDependency(dependency);
		}

		// don't redownload a pre-existing banner
		if (!exists(top10logic) && starts_with(banner, "https://"))
			download(banner, top10logic);
		//else
		//	cout << banner << " doesn't start with https://\n";

		Texture bannertex;
		bannertex.loadFromFile(exists(top10logic) ? top10logic : "content/graphics/nobanner.png");
		modbanners[i] = bannertex;
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


	for (int i = 0; i < mbcount; i++) {
		Sprite minibutton;
		minibutton.setTexture(mm_buttontex);
		minibutton.setTextureRect(IntRect(mbrect_x[i], 146, s2bx, s2by));
		minibuttons[i] = minibutton;
		mbprevhold[i] = false;
	}


	mm_logo.setPosition(Vector2f(400, 0));

	mm_button_launch.setPosition(Vector2f(0, 0));
	mm_button_mods.setPosition(Vector2f(200, 0));
	mm_button_reinstall.setPosition(Vector2f(0, 90));
	mm_button_howtomod.setPosition(Vector2f(200, 90));

	reposscene();
}

void scenesetup_modmenu() {
	if (modmenu_setup)
		return;
	modmenu_setup = true;

	modmenu_buttontex.loadFromFile("content/graphics/mod-buttons.png");

	modmenu_about.setTexture(modmenu_buttontex);
	modmenu_about.setTextureRect(IntRect(600, 0, 192, 80));

	modmenu_install.setTexture(modmenu_buttontex);
	modmenu_install.setTextureRect(installrect);

	modmenu_issues.setTexture(modmenu_buttontex);
	modmenu_issues.setTextureRect(IntRect(800, 0, 192, 80));

	modmenu_sourcecode.setTexture(modmenu_buttontex);
	modmenu_sourcecode.setTextureRect(IntRect(0, 88, 192, 50));

	modmenu_left.setTexture(modmenu_buttontex);
	modmenu_left.setTextureRect(IntRect(0, 146, 67, 67));

	modmenu_right.setTexture(modmenu_buttontex);
	modmenu_right.setTextureRect(IntRect(75, 146, 67, 67));

	modmenu_modbartex.loadFromFile("content/graphics/modbar.png");
	modbar.setTexture(modmenu_modbartex);

	reposscene();
}

void switchstate(StateType newstate) {
	cout << "New state " << newstate << " found.\n";
	StateType oldState = curState;
	curState = newstate;


	switch (newstate) {
		case Main:
			scenesetup_mainmenu();
			verstext.setString(versionstr);
			break;
		case Mods:
			scenesetup_modmenu();
			curmod = 0;
			modmenu_move(0);
			verstext.setString(versionstr + "\nHit escape to go back.");
			break;
	}

	reposscene();
}

void downloadmod(ModData mod, int action) {
	cout << "Modifying " << mod.name << " " << mod.versionname << endl;
	path filedest(aumoddedpath + "/BepInEx/plugins/" + mod.name + ".dll");
	string popuptitle = mod.name + " " + mod.versionname + " - by " + mod.author;
	switch (action) {
		case 0: {
			if (!starts_with(mod.file, "https://")) {
				sfx_appear.play();
				MessageBox(NULL, L"Download link missing!", wstring(popuptitle.begin(), popuptitle.end()).c_str(), MB_ICONERROR);
				sfx_disappear.play();
				modmenu_move(0);
				return;
			}

			// i am so absolutely sorry for this dependency downloader
			if (mod.dependencycount != 0) {
				string aa = "The following dependencies may be downloaded:\n";

				for (int i = 0; i < mod.dependencycount; i++) {
					path funnypath(aumoddedpath + "/BepInEx/plugins/" + mods[i].name + ".dll");
					if (!exists(funnypath))
						aa += "- " + mod.dependencies[i].name + " " + mod.dependencies[i].versionname + "\n";
				}

				if (aa.length() >= 48) {
					sfx_appear.play();
					MessageBox(NULL, wstring(aa.begin(), aa.end()).c_str(), wstring(popuptitle.begin(), popuptitle.end()).c_str(), MB_ICONINFORMATION);
					sfx_disappear.play();

					for (int i = 0; i < mod.dependencycount; i++) {
						path funnypath(aumoddedpath + "/BepInEx/plugins/" + mods[i].name + ".dll");
						if (exists(funnypath))
							continue;
						bool gotit = false;
						for (int o = 0; o < modsactive; o++)
							if (mods[o].name == mod.dependencies[i].name) {
								downloadmod(mods[0], 0);
								gotit = true;
							}
						if (!gotit) {
							string aaa = "Dependency " + mod.dependencies[i].name + " " + mod.dependencies[i].versionname + " not found!\nAborting mod download.";
							sfx_appear.play();
							MessageBox(NULL, wstring(aaa.begin(), aaa.end()).c_str(), wstring(popuptitle.begin(), popuptitle.end()).c_str(), MB_ICONINFORMATION);
							sfx_disappear.play();
							return;
						}
					}
				}
			}

			string filepath = filedest.string();
			download(mod.file, filepath);

			for (int o = 0; o < userdata["mods_installed"].size(); o++)
				if (userdata["mods_installed"][o]["name"] == mod.name) {
					userdata["mods_installed"][o]["last_version"] = mod.version;
					userdata["mods_installed"][o]["last_versionname"] = mod.versionname;
					userdata["mods_installed"][o]["active"] = true;
					saveuserdata();
					modmenu_move(0);
					return;
				}

			int i = userdata["mods_installed"].size();
			userdata["mods_installed"][i]["name"] = mod.name;
			userdata["mods_installed"][i]["last_version"] = mod.version;
			userdata["mods_installed"][i]["last_versionname"] = mod.versionname;
			userdata["mods_installed"][i]["active"] = true;
			saveuserdata();
			modmenu_move(0);

		}
			  break;
		case 1: {
			if (!starts_with(mod.file, "https://")) {
				string t = mod.name + " " + mod.versionname + " - by " + mod.author;
				sfx_appear.play();
				MessageBox(NULL, L"Download link missing!", wstring(t.begin(), t.end()).c_str(), MB_ICONERROR);
				sfx_disappear.play();
				modmenu_move(0);
				return;
			}

			// i am so absolutely sorry for this dependency downloader
			if (mod.dependencycount != 0) {
				string aa = "The following dependencies may be installed:\n";

				for (int i = 0; i < mod.dependencycount; i++) {
					path funnypath(aumoddedpath + "/BepInEx/plugins/" + mods[i].name + ".dll");
					if (!exists(funnypath))
						aa += "- " + mod.dependencies[i].name + " " + mod.dependencies[i].versionname + "\n";
				}
				if (aa.length() >= 48) {
					sfx_appear.play();
					MessageBox(NULL, wstring(aa.begin(), aa.end()).c_str(), wstring(popuptitle.begin(), popuptitle.end()).c_str(), MB_ICONINFORMATION);
					sfx_disappear.play();

					for (int i = 0; i < mod.dependencycount; i++) {
						path funnypath(aumoddedpath + "/BepInEx/plugins/" + mods[i].name + ".dll");
						if (exists(funnypath))
							continue;
						bool gotit = false;
						for (int o = 0; o < modsactive; o++)
							if (mods[o].name == mod.dependencies[i].name) {
								downloadmod(mods[0], 0);
								gotit = true;
							}
						if (!gotit) {
							string aaa = "Dependency " + mod.dependencies[i].name + " " + mod.dependencies[i].versionname + " not found!\nAborting mod download.";
							sfx_appear.play();
							MessageBox(NULL, wstring(aaa.begin(), aaa.end()).c_str(), wstring(popuptitle.begin(), popuptitle.end()).c_str(), MB_ICONINFORMATION);
							sfx_disappear.play();
							return;
						}
					}
				}
			}

			int i = userdata["mods_installed"].size();

			for (int o = 0; o < userdata["mods_installed"].size(); o++)
				if (userdata["mods_installed"][o]["name"] == mod.name) {
					cout << "found ur mod\n";
					i = o;
				}

			userdata["mods_installed"][i]["name"] = mod.name;
			userdata["mods_installed"][i]["last_version"] = mod.version;
			userdata["mods_installed"][i]["last_versionname"] = mod.versionname;
			userdata["mods_installed"][i]["active"] = true;
			saveuserdata();
			modmenu_move(0);

			string filepath = filedest.string();
			download(mod.file, filepath);
		}
			  break;
		case 2:
			if (!exists(filedest)) {
				string t = mod.name + " " + mod.versionname + " - by " + mod.author;
				sfx_appear.play();
				MessageBox(NULL, L"Mod DLL missing!", wstring(t.begin(), t.end()).c_str(), MB_ICONERROR);
				sfx_disappear.play();

				for (int i = 0; i < userdata["mods_installed"].size(); i++)
					if (userdata["mods_installed"][i]["name"] == mod.name)
						userdata["mods_installed"][i]["active"] = false;
				saveuserdata();
				modmenu_move(0);
				return;
			}

			// i am so absolutely sorry for this dependent uninstaller
			string aa = "The following dependents may be uninstalled:\n";

			for (int i = 0; i < modsactive; i++) {
				path funnypath(aumoddedpath + "/BepInEx/plugins/" + mods[i].name + ".dll");
				if (exists(funnypath))
					for (int o = 0; o < mods[i].dependencycount; o++)
						if (mods[i].dependencies[o].name == mod.name)
							aa += "- " + mods[i].name + " " + mods[i].versionname + "\n";
			}

			if (aa.length() >= 48) {
				sfx_appear.play();
				MessageBox(NULL, wstring(aa.begin(), aa.end()).c_str(), wstring(popuptitle.begin(), popuptitle.end()).c_str(), MB_ICONINFORMATION);
				sfx_disappear.play();

				for (int i = 0; i < modsactive; i++) {
					path funnypath(aumoddedpath + "/BepInEx/plugins/" + mods[i].name + ".dll");
					if (exists(funnypath))
						for (int o = 0; o < mods[i].dependencycount; o++)
							if (mods[i].dependencies[o].name == mod.name)
								downloadmod(mods[i], 2);
				}
			}
			//

			for (int i = 0; i < userdata["mods_installed"].size(); i++)
				if (userdata["mods_installed"][i]["name"] == mod.name)
					userdata["mods_installed"][i]["active"] = false;
			saveuserdata();
			modmenu_move(0);

			remove(filedest);
			break;
	}
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

			for (int i = 0; i < mbcount; i++)
				window.draw(minibuttons[i]);
			break;
		case Mods:
			if (!modmenu_setup)
				return;

			window.draw(curmodbanner);


			window.draw(modmenu_about);
			window.draw(modmenu_install);
			window.draw(modmenu_issues);
			window.draw(modmenu_sourcecode);

			window.draw(modmenu_left);
			window.draw(modmenu_right);

			window.draw(modbar);
			window.draw(modnametext);
			break; 
	}

	window.draw(verstext);
	window.display();
}

string wikilink = "https://github.com/DillyzThe1/DillyzRoleApi-Rewritten/wiki";
bool prevpressed = false;
long lastCpuTime = 0;
void update(float secondsPassed) {
	cooldown -= secondsPassed;
	Vector2i mp = getMousePos();
	bool pressing = Mouse::isButtonPressed(Mouse::Left);
	bool justpressed = prevpressed != pressing && pressing;
	switch (curState) {
		case Main: {
			bool hov_launch = !launchdisabled && hoveringSprite(mp, mm_button_launch, 192, 80);
			if (launchdisabled || installingnow || cooldown >= 0)
				mm_button_launch.setColor(color_deselected);

			bool hov_mods = !modsdisabled && hoveringSprite(mp, mm_button_mods, 192, 80);
			if (modsdisabled || installingnow || cooldown >= 0)
				mm_button_mods.setColor(color_deselected);

			bool hov_reinstall = !reinstalldisabled && hoveringSprite(mp, mm_button_reinstall, 192, 50);
			if (reinstalldisabled || installingnow || cooldown >= 0)
				mm_button_reinstall.setColor(color_deselected);

			bool hov_howtomod = !howtodisabled && hoveringSprite(mp, mm_button_howtomod, 192, 50);
			if (howtodisabled || installingnow || cooldown >= 0)
				mm_button_howtomod.setColor(color_deselected);

			if (((hov_launch != prevhov_launch && hov_launch && !launchdisabled)
				|| (hov_mods != prevhov_mods && hov_mods && !modsdisabled)
				|| (hov_reinstall != prevhov_reinstall && hov_reinstall && !reinstalldisabled)
				|| (hov_howtomod != prevhov_howtomod && hov_howtomod && !howtodisabled))
				&& !installingnow && cooldown < 0)
				sfx_hover.play();

			for (int i = 0; i < mbcount; i++) {
				bool mbhold = hoveringSprite(mp, minibuttons[i], 67, 67);
				if (installingnow || cooldown >= 0)
					minibuttons[i].setColor(color_deselected);
				else if (mbhold != mbprevhold[i] && mbhold)
					sfx_hover.play();
				mbprevhold[i] = mbhold;


				if (justpressed && !installingnow && mbhold && cooldown < 0) {
					cooldown = 1.5;
					sfx_select.play();
					switch (mbids[i]) {
						case Announcements:
							showannouncement();
							break;
						case Settings:
							cout << "Display settings.\n";
							break;
						case Innersloth:
							sfx_appear.play();
							ShellExecuteA(NULL, "open", innerslothlink.c_str(), NULL, NULL, SW_SHOWDEFAULT);
							break;
						case Refresh: {
								downloaddata(); 
								ifstream userdata_stream(userdatapath.string());
								userdata = json::parse(userdata_stream);
								userdata_stream.close();
								launchdisabled = modsdisabled = howtodisabled = !userdata["setup_properly"];

								if (userdata["last_announcement"] != announcementjson["version"])
									showannouncement();
							}
							break;
						case Discord:
							sfx_appear.play();
							ShellExecuteA(NULL, "open", discordlink.c_str(), NULL, NULL, SW_SHOWDEFAULT);
							break;
					}
					return;
				}
			}

			if (justpressed && !installingnow && cooldown < 0) {
				if (hov_launch) {
					cooldown = 15;
					sfx_select.play();
					string exepath = aumoddedpath + exename;
					ShellExecuteA(NULL, "open", exepath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					return;
				}
				if (hov_mods) {
					cooldown = 0.5;
					sfx_select.play();
					switchstate(Mods);
					return;
				}
				if (hov_reinstall) {
					cooldown = 1.5;

					// GRAY THE BUTTONS OUT
					mm_button_launch.setColor(color_deselected);
					mm_button_mods.setColor(color_deselected);
					mm_button_reinstall.setColor(color_deselected);
					mm_button_howtomod.setColor(color_deselected);
					for (int i = 0; i < mbcount; i++)
						minibuttons[i].setColor(color_deselected);
					render();
					//

					sfx_select.play();
					sfx_appear.play();
					installingnow = true;
					bool exefound = locateexe();
					cout << "Exe " << (exefound == 1 ? "properly" : "improperly") << " found.\n";
					if (exefound)
						sfx_complete.play();
					else
						sfx_progress.play();
					launchdisabled = modsdisabled = howtodisabled = !userdata["setup_properly"];
					installingnow = false;
					cooldown = 1.5;
					return;
				}
				if (hov_howtomod) {
					cooldown = 1.5;
					sfx_select.play();
					sfx_appear.play();
					//ShellExecuteA(NULL, "open", appdatapath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					ShellExecuteA(NULL, "open", wikilink.c_str(), NULL, NULL, SW_SHOWDEFAULT);
					return;
				}
			}

			prevhov_launch = hov_launch, prevhov_mods = hov_mods, prevhov_reinstall = hov_reinstall, prevhov_howtomod = hov_howtomod;
		}
			break;
		case Mods: {
			bool hov_about = hoveringSprite(mp, modmenu_about, 192, 80);
			bool hov_install = hoveringSprite(mp, modmenu_install, 192, 80);
			bool hov_issues = hoveringSprite(mp, modmenu_issues, 192, 80);
			bool hov_sourcecode = hoveringSprite(mp, modmenu_sourcecode, 192, 50);
			bool hov_left = hoveringSprite(mp, modmenu_left, 67, 67);
			bool hov_right = hoveringSprite(mp, modmenu_right, 67, 67);

			if (curmod < 1)
				modmenu_left.setColor(color_deselected);
			if (curmod >= modsactive - 1)
				modmenu_right.setColor(color_deselected);

			if (installingnow || cooldown >= 0) {
				modmenu_about.setColor(color_deselected);
				modmenu_install.setColor(color_deselected);
				modmenu_issues.setColor(color_deselected);
				modmenu_sourcecode.setColor(color_deselected);
				modmenu_left.setColor(color_deselected);
				modmenu_right.setColor(color_deselected);
			}
			else {
				if ((hov_about != prevhov_about && hov_about) || (hov_install != prevhov_install && hov_install)
					|| (hov_issues != prevhov_issues && hov_issues) || (hov_sourcecode != prevhov_sourcecode && hov_sourcecode)
					|| (hov_left != prevhov_left && hov_left && curmod > 0) || (hov_right != prevhov_right && hov_right && curmod < modsactive - 1))
					sfx_hover.play();

				prevhov_about = hov_about;
				prevhov_install = hov_install;
				prevhov_issues = hov_issues;
				prevhov_sourcecode = hov_sourcecode;
				prevhov_left = hov_left;
				prevhov_right = hov_right;

				if (justpressed) {
					ModData mod = mods[curmod];
					if (hov_about) {
						cooldown = 1.5;
						sfx_select.play();
						string t = mod.name + " " + mod.versionname + " - by " + mod.author;

						modmenu_install.setColor(color_deselected);
						modmenu_issues.setColor(color_deselected);
						modmenu_sourcecode.setColor(color_deselected);
						modmenu_left.setColor(color_deselected);
						modmenu_right.setColor(color_deselected);
						render();

						sfx_appear.play();
						MessageBox(NULL, wstring(mod.description.begin(), mod.description.end()).c_str(), wstring(t.begin(), t.end()).c_str(), MB_ICONINFORMATION);
						sfx_disappear.play();
						return;
					}
					if (hov_install) {
						cooldown = 1.5;
						sfx_select.play();

						modmenu_about.setColor(color_deselected);
						modmenu_issues.setColor(color_deselected);
						modmenu_sourcecode.setColor(color_deselected);
						modmenu_left.setColor(color_deselected);
						modmenu_right.setColor(color_deselected);
						render();


						downloadmod(mod, availableInstallerAction);
						return;
					}
					if (hov_issues) {
						cooldown = 1.5;
						sfx_select.play();
						string funnies = mod.source + "/issues/new";
						sfx_appear.play();
						ShellExecuteA(NULL, "open", funnies.c_str(), NULL, NULL, SW_SHOWDEFAULT);
						return;
					}
					if (hov_sourcecode) {
						cooldown = 1.5;
						sfx_select.play();
						sfx_appear.play();
						ShellExecuteA(NULL, "open", mod.source.c_str(), NULL, NULL, SW_SHOWDEFAULT);
						return;
					}
					if (hov_left && curmod > 0) {
						modmenu_move(-1);
						return;
					}
					if (hov_right && curmod < modsactive - 1) {
						modmenu_move(1);
						return;
					}
				}
			}
		}
			break;
		case Progress:
			break;
	}
	prevpressed = pressing;
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

	Texture icontex;
	icontex.loadFromFile("content/graphics/icon.png");
	window.setIcon(200, 200, icontex.copyToImage().getPixelsPtr());

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
	
	// font
	font_ui.loadFromFile("content/fonts/Roboto-Regular.ttf");
	verstext.setFont(font_ui);
	verstext.setPosition(Vector2f(10, 5));
	verstext.setString(versionstr);
	verstext.setOutlineColor(Color::Black);
	verstext.setOutlineThickness(2);

	font_buttons.loadFromFile("content/fonts/AmaticSC-Bold.ttf");
	modnametext.setFont(font_buttons);
	modnametext.setOutlineColor(Color::Black);
	modnametext.setOutlineThickness(2);
	modnametext.setCharacterSize(60);
	modnametext.setString("DillyzRoleApi");
	//

	curmodbanner.setTexture(modbanners[0]);
	curmodbanner.setTextureRect(IntRect(0, 0, 1280, 480));

	switchstate(Main);
	render();

	if (userdata["last_announcement"] != announcementjson["version"])
		showannouncement();

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
					if (e.key.code == Keyboard::O && (Keyboard::isKeyPressed(Keyboard::LControl) || Keyboard::isKeyPressed(Keyboard::RControl)))
						ShellExecuteA(NULL, "open", appdatapath.c_str(), NULL, NULL, SW_SHOWDEFAULT);

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
								case Keyboard::Left:
									modmenu_move(-1);
									break;
								case Keyboard::Right:
									modmenu_move(1);
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