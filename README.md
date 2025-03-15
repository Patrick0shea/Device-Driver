# Raspberry Pi Roulette Spinner - Device Driver Project

This project implements a Linux Loadable Kernel Module (LKM) and a user-space application to simulate a roulette wheel using GPIO-connected LEDs on a Raspberry Pi.
The module registers a character device driver, allowing user-space programs to interact with it via standard file operations (`open`, `read`, `write`, `ioctl`, `close`) and control
the spinning logic.

---

# Project Structure
- devicedriver.c, Kernel module for GPIO LED Control
- userspace.c, User-space application to control the roulette wheel
- Makefile, builds the kernel module and user app
- gui.py, a GUI for user-accessibility
- README.md, Project documentation

---

# Running the roulette
- Userspace application sends a command to the driver to start spinning. LEDs will light up in a circular pattern, simulating a roulette spin. After a few seconds, one LED will stay on, 
indicating the winning slot.

---

# Notes
- The device driver uses GPIO registers directly for low-level control
- You must run all commands as root when interacting with the device or GPIO
