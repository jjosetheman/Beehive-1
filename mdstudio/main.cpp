///////////////////////////////////////////////////////
// Beehive: A complete SEGA Mega Drive content tool
//
// (c) 2015 Matt Phillips, Big Evil Corporation
///////////////////////////////////////////////////////

#include <windows.h>
#include "Beehive.h"
#include "core/time/Time.h"

int main(char** args, int numargs)
{
	SetDllDirectoryA("bin");

	Beehive app;
	
	if(app.Initialise())
	{
		float deltaTime = 0.0f;
		bool run = true;
		while(run)
		{
			u64 startTicks = ion::time::GetSystemTicks();

			if(run = app.Update(deltaTime))
			{
				app.Render();
			}

			u64 endTicks = ion::time::GetSystemTicks();
			deltaTime = (float)ion::time::TicksToSeconds(endTicks - startTicks);
		}

		app.Shutdown();
	}
}