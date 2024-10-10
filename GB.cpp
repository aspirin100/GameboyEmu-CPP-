#include"GB.h"

const int windowWidth = 160;
const int windowHeight = 144;

static void DoRender()
{
	GB* gb = GB::GetSingleton();
	gb->RenderGame();
}

GB::GB(const std::string& path) :
	m_Emulator(nullptr)
{
	m_Emulator = new Emulator();
	m_Emulator->LoadRom(path);
	m_Emulator->InitGame(DoRender);
}

GB::~GB()
{
	delete m_Emulator;
}

GB* GB::m_Instance = nullptr;
GB* GB::CreateInstance(const std::string& path)
{
	if (!m_Instance)
	{
		m_Instance = new GB(path);
		m_Instance->InitGL();
	}

	return m_Instance;
}

GB* GB::GetSingleton()
{
	return m_Instance;
}

void GB::StartEmulation()
{
	bool quit = false;
	SDL_Event event;

	float fps = 59.73;
	float interval = 1000;
	interval /= fps;

	unsigned int time2 = SDL_GetTicks();

	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			HandleInput(event);

			if (event.type == SDL_QUIT)
			{
				quit = true;
			}
		}

		unsigned int current = SDL_GetTicks();

		if ((time2 + interval) < current)
		{
			m_Emulator->Update();
			time2 = current;
		}

	}
	SDL_Quit();
}


void GB::HandleInput(SDL_Event& event)
{
	if (event.type == SDL_KEYDOWN)
	{
		int key = -1;
		switch (event.key.keysym.sym)
		{
		case SDLK_a: key = 4; break;
		case SDLK_s: key = 5; break;
		case SDLK_RETURN: key = 7; break;
		case SDLK_SPACE: key = 6; break;
		case SDLK_RIGHT: key = 0; break;
		case SDLK_LEFT: key = 1; break;
		case SDLK_UP: key = 2; break;
		case SDLK_DOWN: key = 3; break;
		}
		if (key != -1)
		{
			m_Emulator->KeyPressed(key);
		}
	}
	//If a key was released
	else if (event.type == SDL_KEYUP)
	{
		int key = -1;
		switch (event.key.keysym.sym)
		{
		case SDLK_a: key = 4; break;
		case SDLK_s: key = 5; break;
		case SDLK_RETURN: key = 7; break;
		case SDLK_SPACE: key = 6; break;
		case SDLK_RIGHT: key = 0; break;
		case SDLK_LEFT: key = 1; break;
		case SDLK_UP: key = 2; break;
		case SDLK_DOWN: key = 3; break;
		}
		if (key != -1)
		{
			m_Emulator->KeyReleased(key);
		}
	}
}

void GB::RenderGame()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glRasterPos2i(-1, 1);
	glPixelZoom(1, -1);
	glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, m_Emulator->m_ScreenData);
	SDL_GL_SwapBuffers();
}

bool GB::InitGL()
{

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		return false;
	}
	if (SDL_SetVideoMode(windowWidth, windowHeight, 8, SDL_OPENGL) == NULL)
	{
		return false;
	}

	glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, windowWidth, windowHeight, 0, -1.0, 1.0);
	glClearColor(0, 0, 0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glShadeModel(GL_FLAT);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DITHER);
	glDisable(GL_BLEND);

	SDL_WM_SetCaption("OpenGL Test", NULL);

	return true;
}