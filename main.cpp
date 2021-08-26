#include <iostream>
#include <wiringPi.h>
#include <climits>
#include <unistd.h>
#include <libevdev/libevdev-uinput.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#define PIN_DOWN 0
#define PIN_UP 2
#define PIN_LEFT 3
#define PIN_RIGHT 4
#define PIN_ENTER 5

constexpr int pins[] = {
    PIN_DOWN,
    PIN_UP,
    PIN_LEFT,
    PIN_RIGHT,
    PIN_ENTER
};

constexpr int boundKeys[] = {
    KEY_DOWN,
    KEY_UP,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER
};

static struct libevdev_uinput* udev = nullptr;

static void sendKeyEvent(
    unsigned int code,
    unsigned int pressed
    )
{
    libevdev_uinput_write_event(udev, EV_KEY, code, pressed);
    libevdev_uinput_write_event(udev, EV_SYN, SYN_REPORT, 0);
}

static void sendKey(
    unsigned int code,
    unsigned int modifier
    )
{
    if (modifier != 0)
    {
        sendKeyEvent(modifier, 1);
    }

    sendKeyEvent(code, 1);
    sendKeyEvent(code, 0);

    if (modifier != 0)
    {
        sendKeyEvent(modifier, 0);
    }
}

int pinStates[ARRAY_SIZE(pins)];

static void handleInterrupt(void)
{

#define DOWN (pinStates[0])
#define UP (pinStates[1])
#define LEFT (pinStates[2])
#define RIGHT (pinStates[3])
#define ENTER (pinStates[4])

    for (auto idx = 0; idx < ARRAY_SIZE(pins); ++idx)
    {
        int newValue = digitalRead(pins[idx]);
        if (pinStates[idx] != newValue)
        {
            sendKeyEvent(boundKeys[idx], newValue);
        }

        pinStates[idx] = newValue;
    }

    if (UP && DOWN)
    {
        sendKey(KEY_F4, KEY_LEFTALT);
    }
}

static bool initialize()
{
    int retval;
    struct libevdev* dev = nullptr;

    unsigned int keyCodes[] = {
        KEY_DOWN,
        KEY_UP,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_ENTER,
        KEY_LEFTALT,
        KEY_F4
    };

    dev = libevdev_new();
    libevdev_set_name(dev, "gpio controller");
    if (libevdev_enable_event_type(dev, EV_KEY) != 0)
    {
        perror("Event type set failed");
        goto err;
    }

    for (auto keyCode : keyCodes)
    {
        if (libevdev_enable_event_code(dev, EV_KEY, keyCode, NULL) != 0)
        {
            perror("Unable to set key code");
            goto err;
        }
    }

    retval = libevdev_uinput_create_from_device(
        dev,
        LIBEVDEV_UINPUT_OPEN_MANAGED,
        &udev
    );

    if (retval != 0)
    {
        perror("Unable to create uinput device");
        goto err;
    }

    return true;

err:
    if (dev != nullptr)
    {
        libevdev_free(dev);
    }

    return false;
}

static void deinitialize()
{
    if (udev != nullptr)
    {
        libevdev_uinput_destroy(udev);
    }
}

int main()
{
    if (!initialize())
    {
        perror("Initialization failed");
        goto err;
    }

    if (wiringPiSetup() != 0)
    {
        perror("Unable to initialize wiringPi");
        goto err;
    }

    for (auto pin : pins)
    {
        std::cout << "Made pin " << pin << " as input" << std::endl;
        pinMode(pin, INPUT);
    }

    for (auto pin : pins)
    {
        if (wiringPiISR(pin, INT_EDGE_FALLING, &handleInterrupt) < 0)
        {
            perror("Unable to register interrupt");
            goto err;
        }
    }

    for (;;)
    {
        delay(UINT_MAX);
    }

    deinitialize();
    return 0;

err:
    deinitialize();
    return -1;
}
