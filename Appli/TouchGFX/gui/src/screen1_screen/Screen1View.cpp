#include <gui/screen1_screen/Screen1View.hpp>
#include <touchgfx/Color.hpp>

// External debug variables (defined in main.c)
extern "C" {
    extern volatile uint8_t g_ethLinkStatus;
    extern volatile uint8_t g_soemInitStatus;
    extern volatile uint8_t g_slaveCount;
    extern volatile uint32_t g_soemErrorCode;
}

Screen1View::Screen1View() : 
    tickCounter(0),
    heartbeatState(false),
    lastEthStatus(0xFF),
    lastSoemStatus(0xFF),
    lastErrorCode(0xFFFFFFFF)
{
}

void Screen1View::setupScreen()
{
    Screen1ViewBase::setupScreen();
    
    // Change text color to bright white for visibility
    textProgress1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    textProgress1_1.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    textProgress1_2.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    textProgress1_3.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
    
    // Extend error code range to show actual error values (up to 1200+)
    textProgress1_2.setRange(0, 1500);
    
    // Initial values - force immediate display
    textProgress1.setValue(0);      // ETH: 0=Off, 100=On
    textProgress1.invalidate();
    textProgress1_1.setValue(0);    // SOEM: 0=Init, 50=Working, 100=OK
    textProgress1_1.invalidate();
    textProgress1_2.setValue(0);    // Error Code (0-1500)
    textProgress1_2.invalidate();
    textProgress1_3.setValue(0);    // Heartbeat (0 or 100)
    textProgress1_3.invalidate();
}

void Screen1View::tearDownScreen()
{
    Screen1ViewBase::tearDownScreen();
}

void Screen1View::handleTickEvent()
{
    tickCounter++;
    
    // Update every 30 ticks (~500ms at 60Hz)
    if (tickCounter % 30 == 0)
    {
        // === Always Update ETH Link Status ===
        textProgress1.setValue(g_ethLinkStatus == 1 ? 100 : 0);
        textProgress1.invalidate();
        
        // === Always Update SOEM Status ===
        int soemValue = 0;
        switch (g_soemInitStatus)
        {
            case 0: soemValue = 0; break;   // Not initialized
            case 1: soemValue = 100; break; // OK
            case 2: soemValue = 25; break;  // Error
            default: soemValue = 50; break; // Working/Progress
        }
        textProgress1_1.setValue(soemValue);
        textProgress1_1.invalidate();
        
        // === Always Update Error Code Display ===
        // Show error code directly - range 0-3000
        // 0-999: progress codes (divide by 10 for %)
        // 1000-1999: TX count
        // 2000-2999: RX count
        uint32_t code = g_soemErrorCode;
        textProgress1_2.setRange(0, 3000);
        textProgress1_2.setValue(code > 3000 ? 3000 : code);
        textProgress1_2.invalidate();
        
        // === Heartbeat Toggle ===
        heartbeatState = !heartbeatState;
        textProgress1_3.setValue(heartbeatState ? 100 : 0);
        textProgress1_3.invalidate();
    }
}
