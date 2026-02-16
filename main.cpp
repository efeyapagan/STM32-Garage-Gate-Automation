#include "mbed.h"
using namespace std::chrono;

// -------------------- Pin Definitions --------------------
#define ENTRY_TRIG_PIN   D6
#define ENTRY_ECHO_PIN   D7
#define SERVO_PIN        D9
#define EXIT_TRIG_PIN    D8
#define EXIT_ECHO_PIN    D10

// -------------------- Tunable Parameters --------------------
static constexpr int    DETECT_THRESHOLD_CM = 30;     // Vehicle detected if distance < 30 cm
static constexpr float  SERVO_MIN_ANGLE     = 0.0f;
static constexpr float  SERVO_MAX_ANGLE     = 180.0f;

static constexpr microseconds ULTRA_TIMEOUT = 30ms;   // Max wait for echo transitions
static constexpr microseconds SERVO_STEP_DELAY = 10ms; // Smooth motion
static constexpr seconds GATE_HOLD_OPEN_TIME = 3s;    // Keep gate open before closing (set to 12s if you want)

// -------------------- IO Objects --------------------
DigitalOut entryTrig(ENTRY_TRIG_PIN);
DigitalIn  entryEcho(ENTRY_ECHO_PIN);

DigitalOut exitTrig(EXIT_TRIG_PIN);
DigitalIn  exitEcho(EXIT_ECHO_PIN);

PwmOut servo(SERVO_PIN);

// -------------------- Helper Functions --------------------

// Convert angle (0..180) to pulse width (~1ms..2ms at 50Hz)
static void setServoAngle(float angle_deg) {
    angle_deg = std::max(SERVO_MIN_ANGLE, std::min(SERVO_MAX_ANGLE, angle_deg));
    const float pulse_width_s = 0.001f + (angle_deg / 180.0f) * 0.001f; // 1ms to 2ms
    servo.pulsewidth(pulse_width_s);
}

// Smoothly move servo from start to end angle
static void sweepServo(float start_deg, float end_deg, float step_deg = 1.0f) {
    if (step_deg <= 0.0f) step_deg = 1.0f;

    if (end_deg >= start_deg) {
        for (float a = start_deg; a <= end_deg; a += step_deg) {
            setServoAngle(a);
            ThisThread::sleep_for(SERVO_STEP_DELAY);
        }
    } else {
        for (float a = start_deg; a >= end_deg; a -= step_deg) {
            setServoAngle(a);
            ThisThread::sleep_for(SERVO_STEP_DELAY);
        }
    }
}

// Measure echo pulse duration (microseconds). Returns -1 on timeout.
static int readEchoPulseUs(DigitalOut& trig, DigitalIn& echo, microseconds timeout = ULTRA_TIMEOUT) {
    // Trigger pulse: LOW 2us -> HIGH 10us -> LOW
    trig = 0;
    wait_us(2);
    trig = 1;
    wait_us(10);
    trig = 0;

    Timer t;
    t.start();

    // Wait for echo to go HIGH
    while (!echo.read()) {
        if (t.elapsed_time() > timeout) return -1;
    }

    // Measure HIGH duration
    t.reset();
    while (echo.read()) {
        if (t.elapsed_time() > timeout) return -1;
    }

    return (int)t.elapsed_time().count(); // microseconds
}

// Convert pulse duration to distance (cm). Returns -1 on timeout/error.
static int readDistanceCm(DigitalOut& trig, DigitalIn& echo) {
    const int pulse_us = readEchoPulseUs(trig, echo);
    if (pulse_us < 0) return -1;

    // Speed of sound approx. 343 m/s.
    // Distance(cm) = (pulse_us * 0.0343) / 2 = pulse_us * 0.01715
    const float cm = pulse_us * 0.01715f;
    return (int)(cm + 0.5f); // round to nearest int
}

static bool isVehicleDetected(DigitalOut& trig, DigitalIn& echo, int threshold_cm = DETECT_THRESHOLD_CM) {
    const int cm = readDistanceCm(trig, echo);
    if (cm < 0) return false; // treat timeout as "no detection"
    return cm < threshold_cm;
}

// -------------------- Main --------------------
int main() {
    // Servo standard: 50Hz
    servo.period(0.02f);

    // Start gate closed
    setServoAngle(SERVO_MIN_ANGLE);

    while (true) {
        // 1) Check entry sensor
        if (isVehicleDetected(entryTrig, entryEcho)) {
            printf("Entry detected. Opening gate...\n");

            // 2) Open gate smoothly
            sweepServo(SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

            // 3) Wait for exit confirmation
            printf("Waiting for exit sensor...\n");
            while (true) {
                if (isVehicleDetected(exitTrig, exitEcho)) {
                    printf("Exit detected.\n");
                    break;
                }
                ThisThread::sleep_for(100ms);
            }

            // 4) Keep open for a while, then close
            printf("Holding gate open, then closing...\n");
            ThisThread::sleep_for(GATE_HOLD_OPEN_TIME);

            sweepServo(SERVO_MAX_ANGLE, SERVO_MIN_ANGLE);

            // Small cooldown to avoid instant re-triggering
            ThisThread::sleep_for(500ms);
            printf("Gate closed.\n");
        }

        ThisThread::sleep_for(50ms);
    }
}
