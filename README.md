# Alarm-Monitor

A project to create a simple SmartThings compatible device that can detect a non-smart smoke alarm.

## Introduction

SmartThings provides a great ecosystem for integrating several disparate smart
devices into a single experience. For safety devices specifically (e.g. alarms,
water sensors, etc.) it has some great features for notifying the user that
there is an issue.

Not wanting to replace all 6 existing "wireless" smoke alarms that I already
have with ones compatible with SmartThings, I set out in a quest to create
a simple device that could serve as a sensor for the existing smoke alarms
I already have.

In this quest I decided to share my experience and the source so others looking
to do similar projects might be able to get a head start.

## Background

This project leverages a simple [ESP8266](https://www.adafruit.com/product/3046)
device. If you have not worked with programmable electronic devices, such as
Arduino or Raspberry Pi, this project might serve as an introduction. The ESP8266
device is tiny (2" x 0.9") and supports WiFi connectivity. It also supports the
ability to run as a Web-server, which is a key requirement to integrate with
the SmartThings platform.

The idea is to use the ESP8266 as a device emulating a smoke detector for the
SmartThings platform. It will detect the sound or alarm of the smoke detector
via a microphone circuit connects to the ESP8266 device.

## Current State of the Project

This project is still in-development, but much of the heavy lifting has already
been completed.

**Tasks Remaining**

- Connect microphone circuit to ESP8266
- Update the ESP8266 programming to detect the alarm sound
- Document additional details

**Tasks Completed**

- Program the ESP8266 device (minus microphone support)
- Create the SmartThings "SmartApp" used to detect/locate the device dynamically
and allow the user to add it to their list of devices
- Create a SmartThings "Device Handler" used to managed the state of the device
detecting when the alarm is sounding and indicating that the smoke alarm detected
smoke

The tasks that have been completed thus far has taken a fair amount of effort and
time. This is primarily due to the lack of detail in the SmartThings documentation.
It took much more time to get the features and workflow working the way I wanted
within SmartThings. This doesn't necessarily mean that SmartThings isn't a great
platform for this - it simply means the documentation suffers from the depth
necessary for a seasoned software developer to work with the platform easily. It
required a substantial amount of trial and error to figure out all of the moving
parts and pieces to get this far.
