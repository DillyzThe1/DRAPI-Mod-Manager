#include <iostream>
#include <SFML/Graphics.hpp>
#include <time.h>

using namespace std;
using namespace sf;

VideoMode video(1280, 720);
RenderWindow window(video, "DRAPI Mod Manager for Among Us");

Color bgcolor(15, 10, 25, 255);

void render() {
	window.clear(bgcolor);

	window.display();
}

int main() {
	cout << "There is a pipebomb in your mailbox.\n";

	window.setFramerateLimit(120);
	window.setVerticalSyncEnabled(true);
	window.requestFocus();

	while (window.isOpen()) {
		Event e;
		while (window.pollEvent(e)) {
			switch (e.type) {
				case Event::Closed:
					window.close();
					break;
				case Event::KeyPressed:
					switch (e.key.code) {
						case Keyboard::Escape:
							window.close();
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