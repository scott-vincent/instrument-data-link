const int MaxJoysticks = 16;
const int MaxAxes = 6;
const int MaxButtons = 32;

struct Joystick
{
	int mid;
	int pid;
	char name[32];
	int axisCount;
	int buttonCount;
	bool initialised;
	bool zeroed;
	int axisZero[MaxAxes];
	int axis[MaxAxes];
	int button[MaxButtons];
};

void initJoysticks();
void refreshJoysticks();
void joyRefresh(int id);
