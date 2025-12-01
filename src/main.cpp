#include "EnvMonitor.h"

// ---- Tick Timer ----
constexpr unsigned long TICK_INTERVAL_MS = 1000;   // 1 s
volatile bool ticked = false;                      // set by the timer ISR

// Hardware timer
hw_timer_t * timer = NULL;

void setupTimer(unsigned long interval_ms, void (*isr_callback)()) {
  // Timer index 0 to 3 (ESP32 has 4 hardware timers)
  timer = timerBegin(0, 80, true); // Timer 0, prescaler 80 (1 tick = 1us)
  
  // Attach the interrupt service routine (ISR) to the timer
  timerAttachInterrupt(timer, isr_callback, true);
  
  // Set the timer alarm to trigger after the specified interval
  // interval_ms is in milliseconds, so we convert to microseconds
  timerAlarmWrite(timer, interval_ms * 1000, true); // true means it repeats
  
  // Enable the timer
  timerAlarmEnable(timer);
}

void IRAM_ATTR onTimer()
{
    ticked = true;          // flag the main loop
}

#if defined(MODE_NODE)
// Callback function to notify that transmission is complete
void onPacketSent()
{
}

EnvMonitor envMonitor(onPacketSent, nullptr);
#elif defined(MODE_GATEWAY)
bool packet_received = false;
static String payload = "0";
void onPacketReceived()
{
    packet_received = true; // set flag indicating packet received
}

EnvMonitor envMonitor(nullptr, onPacketReceived);
#endif


/* ---------- Setup ------------------------------------- */
void setup()
{
    envMonitor.init(CONFIG_RS232_BAUD);
    setupTimer(1000, onTimer);
}

/* ---------- Main loop --------------------------------- */
void loop()
{
    // Wait until a timer tick occurs
    if (ticked)
    {
        ticked = false;       // reset flag
        envMonitor.tick();    // do necessary processing
    }
}
