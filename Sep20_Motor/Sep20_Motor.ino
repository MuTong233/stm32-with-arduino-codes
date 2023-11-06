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

// Consts
const char LEDPort = PC13;
const char IN1 = PA0;
const char IN2 = PA1;
const char IN3 = PA2;
const char IN4 = PA3;
const char echoPin = PA6;
const char trigPin = PA7;
const unsigned int MAX_MESSAGE_LENGTH = 12;

// Variants
unsigned int MIN_DISTANCE, MAX_DISTANCE;
unsigned int SENSE = 4;
bool SONIC = false, MOTOR = false, DISRPT = true, WORK = true, ANTICLOCK = false;
float duration, distance;

// Select Running speed and Radar Sensitive
void setSensitive(int num) {
  switch (num) {
    case 1:
      MIN_DISTANCE = 14;
      MAX_DISTANCE = 320;
      break;
    case 2:
      MIN_DISTANCE = 20;
      MAX_DISTANCE = 280;
      break;
    case 3:
      MIN_DISTANCE = 28;
      MAX_DISTANCE = 250;
      break;
    case 4:
      MIN_DISTANCE = 39;
      MAX_DISTANCE = 200;
      break;
    case 5:
      MIN_DISTANCE = 45;
      MAX_DISTANCE = 200;
      break;
    case 6:
      MIN_DISTANCE = 50;
      MAX_DISTANCE = 200;
      break;
    case 7:
      MIN_DISTANCE = 55;
      MAX_DISTANCE = 200;
      break;
    default:
      Serial.println("Set Sensitivity Failed! Please reset default!");
      WORK = false;
  }
}

// Stepper Motor Work function
// Must be executed in a thread, otherwise will throw exception.
void stepclock(int num, int during) {
  if (ANTICLOCK == false) {
    for (int count = 0; count < num; count++) {
      digitalWrite(IN1, HIGH);
      coop_idle(during);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      coop_idle(during);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      coop_idle(during);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      coop_idle(during);
      digitalWrite(IN4, LOW);
    }
  } else {
    for (int count = 0; count < num; count++) {
      digitalWrite(IN4, HIGH);
      coop_idle(during);
      digitalWrite(IN4, LOW);
      digitalWrite(IN3, HIGH);
      coop_idle(during);
      digitalWrite(IN3, LOW);
      digitalWrite(IN2, HIGH);
      coop_idle(during);
      digitalWrite(IN2, LOW);
      digitalWrite(IN1, HIGH);
      coop_idle(during);
      digitalWrite(IN1, LOW);
    }
  }
}

// String to Integer function (as the ARM Compiler doesn't support switch str yet.)
constexpr unsigned int str2int(const char *str, int h = 0) {
  return !str[h] ? 5381 : (str2int(str, h + 1) * 33) ^ str[h];
}

// Thread 1: Supersonic Detection
extern "C" void proc_worker(void *arg) {
  while (1) {
    // Hello
    coop_yield();
    // Below Section is for the main purpose.
    setSensitive(SENSE);
    if (WORK == true) {
      // Supersonic Detection
      digitalWrite(trigPin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);
      duration = pulseIn(echoPin, HIGH);
      distance = (duration * .0343) / 2;

      // Supersonic Debug Information
      if (SONIC == true) {
        Serial.print("[Sonic] Distance: ");
        Serial.println(distance);
      }
    }
  }
}

extern "C" void proc_motor(void *arg) {
  while (1) {
    // Tell worker I'm fine and run thread with LED cleared.
    coop_yield();
    digitalWrite(LEDPort, HIGH);
    // Stepper Motor Run Conditionally
    if (WORK == true) {
      if (distance < MIN_DISTANCE) {  // When reaching distance limit, slow down the motor.
        if (MOTOR == true) {
          Serial.println("[Motor] Clockwise Running Slowly");
        }
        digitalWrite(LEDPort, LOW);  // Also turn on the LED
        stepclock(32, 8);
      } else {
        if (distance > MAX_DISTANCE && DISRPT == true) {  // If Disruption is not allowed and the distance is larger than max allowed.
          if (MOTOR == true) {
            Serial.println("[Motor] Disrupted, not spinning");
          }
          for (int count = 0; count < 8; count++) {
            // Blink LED to warn user.
            digitalWrite(LEDPort, LOW);
            coop_idle(100);
            digitalWrite(LEDPort, HIGH);
            coop_idle(100);
          }
        } else {  // Motor running normally.
          if (MOTOR == true) {
            Serial.println("[Motor] Clockwise Running");
          }
          stepclock(64, 3);
        }
      }
    } else {
      // This section will execute when the working status set to false.
      digitalWrite(LEDPort, LOW);
      coop_idle(100);
      digitalWrite(LEDPort, HIGH);
      coop_idle(100);
    }
  }
}

extern "C" void proc_serial(void *arg) {
  while (1) {
    // Hello
    coop_yield();
    // Below Section is for Serial Communication.
    while (Serial.available() > 0) {
      // Create a place to hold the incoming message
      static char message[MAX_MESSAGE_LENGTH];
      static unsigned int message_pos = 0;

      // Read the next available byte in the serial receive buffer
      char inByte = Serial.read();

      // Message coming in (check not terminating character) and guard for over message size
      if (inByte != '\n' && (message_pos < MAX_MESSAGE_LENGTH - 1)) {
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
          case str2int("ENUM"):
            Serial.println("Current Configuration:");
            Serial.print("Motor Debug: ");
            Serial.println(MOTOR);
            Serial.print("Sonic Debug: ");
            Serial.println(SONIC);
            Serial.print("Break on Disruption: ");
            Serial.println(DISRPT);
            Serial.print("Working: ");
            Serial.println(WORK);
            Serial.print("Distance Range: ");
            Serial.print(MIN_DISTANCE);
            Serial.print(" to ");
            Serial.println(MAX_DISTANCE);
            Serial.print("Run Anticlockwisely: ");
            Serial.println(ANTICLOCK);
            break;
          case str2int("SONIC"):
            Serial.println("OK");
            SONIC = !SONIC;
            break;
          case str2int("MOTOR"):
            Serial.println("OK");
            MOTOR = !MOTOR;
            break;
          case str2int("DISRPT"):
            Serial.println("OK");
            DISRPT = !DISRPT;
            break;
          case str2int("PAUSE"):
            Serial.println("OK");
            WORK = !WORK;
            break;
          case str2int("ANTI"):
            Serial.println("OK");
            ANTICLOCK = !ANTICLOCK;
            break;
          case str2int("UP"):
            if (SENSE < 7) {
              Serial.println("OK");
              SENSE++;
            } else {
              Serial.println("NG");
            }
            break;
          case str2int("DN"):
            if (SENSE > 1) {
              Serial.println("OK");
              SENSE--;
            } else {
              Serial.println("NG");
            }
            break;
          case str2int("RESET"):
            Serial.println("Resetting Default Settings");
            WORK = true;
            SONIC = false;
            MOTOR = false;
            DISRPT = true;
            ANTICLOCK = false;
            SENSE = 4;
            break;
          case str2int("HELP"):
            Serial.println("Available Commands:");
            Serial.println("ENUM, SONIC, MOTOR, DISRPT, PAUSE, RESET, UP, DN, ANTI");
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

// Device Initialization
void setup() {
  // Initialize Serial Communication
  Serial.setRx(PA10);
  Serial.setTx(PA9);
  Serial.begin(9600);
  Serial.println("Started Serial Port on PA9 and PA10.");
  Serial.println("Device: STM32F1xx");
  Serial.println("Code by MuTong233 from DLNU");
  Serial.println("All rights reserved.");
  // Initialize Ports
  Serial.println("Port Initialize");
  pinMode(LEDPort, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
}

// Main code here, to run repeatedly:
void loop() {
  Serial.println("Starting Code Execution in Multithread Mode");
  // Schedule to run 3 workers in parallel mode
  coop_sched_thread(proc_worker, "thrd_1", THREAD_STACK_SIZE, (void *)1);
  coop_sched_thread(proc_serial, "thrd_2", THREAD_STACK_SIZE, (void *)1);
  coop_sched_thread(proc_motor, "thrd_3", THREAD_STACK_SIZE, (void *)1);
  // Start the service
  coop_sched_service();
  // Normally those threads won't exit because they are running in a loop.
  // If there is a problem, the system must tell user there is something wrong.
  Serial.println("3 Thread Exited Abnormally, Restarting.");
}
