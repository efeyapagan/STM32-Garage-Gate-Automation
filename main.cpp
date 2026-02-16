#include "mbed.h"
using namespace std::chrono;

// Pinler
#define ENTRY_TRIG_PIN   D6
#define ENTRY_ECHO_PIN   D7
#define SERVO_PIN        D9
#define EXIT_TRIG_PIN    D8
#define EXIT_ECHO_PIN    D10

// Ayarlar
static constexpr int DETECT_THRESHOLD_CM = 30;
static constexpr float SERVO_MIN_ANGLE = 0.0f;
static constexpr float SERVO_MAX_ANGLE = 180.0f;

static constexpr microseconds ULTRA_TIMEOUT = 30ms;
static constexpr microseconds SERVO_STEP_DELAY = 10ms;
static constexpr seconds GATE_HOLD_OPEN_TIME = 3s;

// Donanım Nesneleri
DigitalOut entryTrig(ENTRY_TRIG_PIN);
DigitalIn  entryEcho(ENTRY_ECHO_PIN);

DigitalOut exitTrig(EXIT_TRIG_PIN);
DigitalIn  exitEcho(EXIT_ECHO_PIN);

PwmOut servo(SERVO_PIN);

// Servo açı ayarı
static void setServoAngle(float angle_deg) {
    angle_deg = std::max(SERVO_MIN_ANGLE, std::min(SERVO_MAX_ANGLE, angle_deg));
    float pulse_width = 0.001f + (angle_deg / 180.0f) * 0.001f;
    servo.pulsewidth(pulse_width);
}

// Servo yumuşak hareket
static void sweepServo(float start_deg, float end_deg, float step_deg = 1.0f) {
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

// Ultrasonik süre ölçümü
static int readEchoPulseUs(DigitalOut& trig, DigitalIn& echo) {
    trig = 0;
    wait_us(2);
    trig = 1;
    wait_us(10);
    trig = 0;

    Timer t;
    t.start();

    while (!echo.read()) {
        if (t.elapsed_time() > ULTRA_TIMEOUT) return -1;
    }

    t.reset();
    while (echo.read()) {
        if (t.elapsed_time() > ULTRA_TIMEOUT) return -1;
    }

    return (int)t.elapsed_time().count();
}

// Mesafe hesaplama (cm)
static int readDistanceCm(DigitalOut& trig, DigitalIn& echo) {
    int pulse_us = readEchoPulseUs(trig, echo);
    if (pulse_us < 0) return -1;

    float cm = pulse_us * 0.01715f;
    return (int)(cm + 0.5f);
}

// Araç algılama
static bool isVehicleDetected(DigitalOut& trig, DigitalIn& echo) {
    int cm = readDistanceCm(trig, echo);
    if (cm < 0) return false;
    return cm < DETECT_THRESHOLD_CM;
}

int main() {
    servo.period(0.02f);  // 50Hz
    setServoAngle(SERVO_MIN_ANGLE);  // Başlangıç kapalı

    while (true) {

        if (isVehicleDetected(entryTrig, entryEcho)) {

            sweepServo(SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

            while (true) {
                if (isVehicleDetected(exitTrig, exitEcho)) {
                    break;
                }
                ThisThread::sleep_for(100ms);
            }

            ThisThread::sleep_for(GATE_HOLD_OPEN_TIME);

            sweepServo(SERVO_MAX_ANGLE, SERVO_MIN_ANGLE);

            ThisThread::sleep_for(500ms);
        }

        ThisThread::sleep_for(50ms);
    }
}
