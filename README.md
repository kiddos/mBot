# mblock nano firmware

[![Build Status](https://travis-ci.org/kiddos/mBot.svg?branch=master)](https://travis-ci.org/kiddos/mBot)

Project forked from mBot default Firmware specifically for user to easily use makeblock IDE for serial communications

The project uses [platformio](http://platformio.org/) as build system.
Install platformio using pip

```shell
pip install -U platformio
```

### LED strip

```
cd ledstrip
platformio run --target upload
```

### Otto Firmware

```shell
cd otto-mblock
platformio run --target upload
```

Firmware for the [Otto Robot](http://otto.strikingly.com/)

* Modules
  - 4 servos
  - 1 buzzer
  - 1 ultrasonic

### Otto Remote Control

write the firmware in otto-robot

```shell
cd otto-remote
platformio run --target upload
```

build the android app using `gradlew`

```
cd OttoRemoteClient
./gradlew
```
