#include <iostream>
#include <SFML/Graphics.hpp>
#include <time.h>
#include "tinyfiledialogs.h"

using namespace std;
using namespace sf;

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

// SETUP SCENE VARS
// TITLE SCENE VARS
// MAIN MENU SCENE VARS
// MODS MENU SCENE VARS
// PROGRESS SCENE SCENE VARS
// :sadsping:

void switchstate(StateType newstate) {
	cout << "New state " << newstate << " found.\n";
	StateType oldState = curState;
	curState = newstate;
}

char const* AmongUsExeFilter[1] = { "Among Us.exe" };
void locateexe() {
	string funny = tinyfd_openFileDialog("finding mungus", "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Among Us\\", 1, AmongUsExeFilter, "among us exe", 0);
	cout << funny << "\n";
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

// this function will check if you're downloading/copying/installing anything and then close the window if not
void close() {
	window.close();
}

int main() {
	cout << "There is a pipebomb in your mailbox.\n";

	window.setFramerateLimit(120);
	window.setVerticalSyncEnabled(true);
	window.requestFocus();

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
								case Keyboard::E:
									locateexe();
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