// MuTong233's shit code
// Using Arduino IDE cuz i'm lazy to learn uVision :cool:
// Begin
// Multithread Configuration
#include "coop_threads.h"
#define CONFIG_IDLE_CB_ALT
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#define THREAD_STACK_SIZE 0x400U
#elif ARDUINO_ARCH_AVR
#define THREAD_STACK_SIZE 0x80U
#else
#define THREAD_STACK_SIZE 0
#endif
#if CONFIG_MAX_THREADS < 5
#error CONFIG_MAX_THREADS >= 5 is required
#endif
#if !CONFIG_OPT_IDLE
#error CONFIG_OPT_IDLE need to be configured
#endif

constexpr unsigned int str2int(const char *str, int h = 0) {
  return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

extern "C" void proc_serial(void *arg) {
  while (1) {
    // Hello
    coop_yield();
    // Below Section is for Serial Communication.
    while (Serial.available() > 0) {
      // Create a place to hold the incoming message
      static char message[32];
      static unsigned int message_pos = 0;

      // Read the next available byte in the serial receive buffer
      char inByte = Serial.read();

      // Message coming in (check not terminating character) and guard for over message size
      if (inByte != '\n' && (message_pos < 32 - 1)) {
        // Add the incoming byte to our message
        message[message_pos] = inByte;
        message_pos++;
      }
      // Full message received...
      else {
        // Add null character to string
        message[message_pos] = '\0';
        // Check the message and do things.
        switch (str2int(message)) {
          case str2int("SHUTDOWN"):
            Serial.println("OK");
            return;
          case str2int("HELP"):
            Serial.println("Available Commands:");
            Serial.println("SHUTDOWN");
            break;
          default:
            Serial.print("[Serial] Unknown Command: ");
            Serial.print(message);
            Serial.println(".");
        }
        // Reset for the next message
        message_pos = 0;
      }
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  // Initialize Serial Communication
  Serial.setRx(PA10);
  Serial.setTx(PA9);
  Serial.begin(9600);
  Serial.println("Started Serial Port on PA9 and PA10.");
  Serial.println("Initialized minimal system.");
  // Initialize Ports
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("Starting Code Execution in Multithread Mode");
  // Schedule to run workers in parallel mode
  coop_sched_thread(proc_serial, "thrd_1", THREAD_STACK_SIZE, (void *)1);
  // Start the service
  coop_sched_service();
  // Normally those threads won't exit because they are running in a loop.
  // If there is a problem, the system must tell user there is something wrong.
  Serial.println("System Service Ended.");
  while(1)
  {
  }
}
