#pragma once
#include"Emulator.h"
#include"SDL.h"
#include"SDL_opengl.h"
#include<stdio.h>


class GB
{
public:
	static GB* CreateInstance(const std::string&);
	static GB* GetSingleton();



	void StartEmulation();
	void RenderGame();

	~GB();

private:

	GB(const std::string&);

	Emulator* m_Emulator = nullptr;
	static GB* m_Instance;
	
	
	void HandleInput(SDL_Event& event);
	bool InitGL();
};