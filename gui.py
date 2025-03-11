import tkinter as tk
from tkinter import scrolledtext
import subprocess
import re
import threading
import os

# Automatically detect the script's directory
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Paths
DRIVER_DIR = SCRIPT_DIR  # Use detected script directory
MODULE_NAME = "devicedriver.ko"
USERSPACE_FILE = "userspace.c"
DEVICE_PATH = "/dev/devicedriver"
PROC_FILE = "/proc/roulette_winner"

def update_terminal(text):
    """Append text to the terminal display."""
    terminal.insert(tk.END, text + "\n")
    terminal.see(tk.END)  # Auto-scroll to latest output

def build_driver():
    """Runs `make` to compile the driver."""
    
    update_terminal("\n[INFO] Running: make\n")
    process = subprocess.Popen("make", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=DRIVER_DIR)

    for line in process.stdout:
        update_terminal(line.strip())
    for line in process.stderr:
        update_terminal(line.strip())

    process.wait()
    update_terminal("\n[INFO] Make Complete\n")

def load_driver():
    """
    - Loads the driver
    - creates the device file
    """
    
    update_terminal("\n[INFO] Running: sudo insmod devicedriver.ko\n")
    subprocess.run(f"sudo insmod {DRIVER_DIR}/{MODULE_NAME}", shell=True)

    update_terminal("\n[INFO] Checking major number...\n")
    dmesg_output = subprocess.run("dmesg | tail -20", shell=True, capture_output=True, text=True).stdout
    match = re.search(r"Allocated major number (\d+)", dmesg_output)

    if match:
        major_number = match.group(1)
        update_terminal(f"[INFO] Major number detected: {major_number}")
        update_terminal(f"\n[INFO] Running: sudo mknod {DEVICE_PATH} c {major_number} 0\n")
        process = subprocess.run(f"sudo mknod {DEVICE_PATH} c {major_number} 0", shell=True)
        process.wait()
        update_terminal("\n[INFO] Driver Loaded Successfully\n")
    else:
        update_terminal("[ERROR] Could not detect major number. Check dmesg manually.")

def run_userspace():
    """Compiles and runs the userspace application."""
    
    update_terminal("\n[INFO] Running: gcc userspace.c -o userspace -lpthread\n")
    process = subprocess.Popen(f"gcc {DRIVER_DIR}/{USERSPACE_FILE} -o {DRIVER_DIR}/userspace -lpthread", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    for line in process.stdout:
        update_terminal(line.strip())
    for line in process.stderr:
        update_terminal(line.strip())

    process.wait()
    
    update_terminal("\n[INFO] Running: sudo ./userspace\n")
    process = subprocess.Popen(f"sudo {DRIVER_DIR}/userspace", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    for line in process.stdout:
        update_terminal(line.strip())
    for line in process.stderr:
        update_terminal(line.strip())

    process.wait()

def unload_driver():
    """
    - Unloads the driver
    - removes the device file
    - cleans the build.
    """
    
    update_terminal("\n[INFO] Running: sudo rmmod devicedriver\n")
    subprocess.run("sudo rmmod devicedriver", shell=True)

    update_terminal("\n[INFO] Running: sudo rm /dev/devicedriver\n")
    subprocess.run("sudo rm /dev/devicedriver", shell=True)

    update_terminal("\n[INFO] Running: make clean\n")
    process = subprocess.Popen("make clean", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=DRIVER_DIR)

    for line in process.stdout:
        update_terminal(line.strip())
    for line in process.stderr:
        update_terminal(line.strip())

    process.wait()
    update_terminal("\n[INFO] Driver Unloaded Successfully\n")

def read_proc_file():
    """Reads the /proc/roulette_stats file and displays its contents."""
    update_terminal("\n[INFO] Reading /proc/roulette_stats...\n")
    
    try:
        with open(PROC_FILE, "r") as file:
            content = file.read().strip()
        update_terminal(f"[INFO] Proc File Content:\n{content}\n")
    except Exception as e:
        update_terminal(f"[ERROR] Failed to read {PROC_FILE}: {e}")

    

# Create Tkinter GUI
root = tk.Tk()
root.title("Raspberry Pi Roulette")

frame = tk.Frame(root, padx=20, pady=20)
frame.pack(pady=20)

title_label = tk.Label(frame, text="Raspberry Pi Roulette", font=("Arial", 16, "bold"))
title_label.pack()

build_button = tk.Button(frame, text="Build Driver", font=("Arial", 12), command=lambda: threading.Thread(target=build_driver, daemon=True).start())
build_button.pack(pady=5)

load_button = tk.Button(frame, text="Load Driver", font=("Arial", 12), command=lambda: threading.Thread(target=load_driver, daemon=True).start())
load_button.pack(pady=5)

run_button = tk.Button(frame, text="Spin Wheel", font=("Arial", 12), command=lambda: threading.Thread(target=run_userspace, daemon=True).start())
run_button.pack(pady=5)

read_proc_button = tk.Button(frame, text="Read /proc File", font=("Arial", 12), command=lambda: threading.Thread(target=read_proc_file, daemon=True).start())
read_proc_button.pack(pady=5)

unload_button = tk.Button(frame, text="Unload Module", font=("Arial", 12), command=lambda: threading.Thread(target=unload_driver, daemon=True).start())
unload_button.pack(pady=5)

# Terminal Output Display
terminal_label = tk.Label(frame, text="Terminal Output:", font=("Arial", 12))
terminal_label.pack(pady=5)

terminal = scrolledtext.ScrolledText(frame, width=60, height=15, font=("Courier", 10))
terminal.pack()

# Run the Tkinter event loop
root.mainloop()
