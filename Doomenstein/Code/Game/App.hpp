#pragma once

//-----------------------------------------------------------------------------------------------
class Game;

//-----------------------------------------------------------------------------------------------
class App
{
public:
    App();
    ~App();
    void Startup();
    void Shutdown();
    void RunMainLoop();
    void RunFrame();

    void HandleQuitRequested();
    bool IsQuitting() const { return m_isQuitting; }

private:
    void BeginFrame();
    void Update();
    void Render() const;
    void EndFrame();
    
    void LoadGameConfig(char const* gameConfigXmlFilePath);

private:
    double m_timeLastFrameStart = 0.0f;
    bool m_isQuitting = false;
};
