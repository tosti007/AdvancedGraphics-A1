#pragma once

#include "surface.h"
#include "camera.h"
#include "primitive.h"

namespace AdvancedGraphics {

class Game
{
public:
	void SetTarget( Surface* surface ) { screen = surface; }
	void Init();
	void Shutdown();
	void Tick( float deltaTime );
	
	void MouseUp( int button ) { view->MouseUp(button); }
	void MouseDown( int button ) { view->MouseDown(button); }
	void MouseMove( int x, int y ) { view->MouseMove(x, y); }
	void KeyUp( int key, byte repeat ) { view->KeyUp(key, repeat); }
	void KeyDown( int key, byte repeat ) { view->KeyDown(key, repeat); }

	bool Intersect( Ray* r );
	Pixel Trace( Ray* r, uint depth );
private:
	Surface* screen;
	Camera* view;
	Sphere* sphere;
	Plane* floor;
	Triangle* trian;
};

}; // namespace AdvancedGraphics