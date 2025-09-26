#include"GB.h"
#include <iostream>

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
	if (m_GLContext) SDL_GL_DestroyContext(m_GLContext);
    if (m_Window) SDL_DestroyWindow(m_Window);
    SDL_Quit();
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

			if (event.type == SDL_EVENT_QUIT)
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
	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		int key = -1;
		switch (event.key.scancode)
		{
		case SDL_SCANCODE_A: key = 4; break;
		case SDL_SCANCODE_S: key = 5; break;
		case SDL_SCANCODE_RETURN: key = 7; break;
		case SDL_SCANCODE_SPACE: key = 6; break;
		case SDL_SCANCODE_RIGHT: key = 0; break;
		case SDL_SCANCODE_LEFT: key = 1; break;
		case SDL_SCANCODE_UP: key = 2; break;
		case SDL_SCANCODE_DOWN: key = 3; break;
		}
		if (key != -1)
		{
			m_Emulator->KeyPressed(key);
		}
	}
	//If a key was released
	else if (event.type == SDL_EVENT_KEY_UP)
	{
		int key = -1;
		switch (event.key.scancode)
		{
		case SDL_SCANCODE_A: key = 4; break;
		case SDL_SCANCODE_S: key = 5; break;
		case SDL_SCANCODE_RETURN: key = 7; break;
		case SDL_SCANCODE_SPACE: key = 6; break;
		case SDL_SCANCODE_RIGHT: key = 0; break;
		case SDL_SCANCODE_LEFT: key = 1; break;
		case SDL_SCANCODE_UP: key = 2; break;
		case SDL_SCANCODE_DOWN: key = 3; break;
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
	glDrawPixels(windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, m_Emulator->m_ScreenData);
	SDL_GL_SwapWindow(m_Window);
}

bool GB::InitGL()
{

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << SDL_GetError();
		return false;
	}

	m_Window = SDL_CreateWindow("OpenGL Test", windowWidth, windowHeight, SDL_WINDOW_OPENGL);
	if(!m_Window){
		std::cout << SDL_GetError();
		return false;
	}
				
	m_GLContext = SDL_GL_CreateContext(m_Window);
	if(!m_GLContext){
		std::cout << SDL_GetError();
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

	return true;
}