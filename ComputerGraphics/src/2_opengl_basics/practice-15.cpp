#define CG_APPLICATION_CUSTOM_CALLBACK_KEY

#include <CG.h>

int main()
{
	Application app{ 800, 800 };
	int code = app.Init("i hate computer graphics 2");
	if (code) return code;
	app.Run();
}

