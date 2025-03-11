import tkinter as tk
from tkinter import scrolledtext, messagebox
import os
import subprocess
import time
import threading

# Paths
DRIVER_DIR = "/home/hoodie/Device-Driver"  # Change this to your driver directory
DEVICE_PATH = "/dev/devicedriver"
MODULE_NAME = "devicedriver"
MODULE_PATH = f"{DRIVER_DIR}/devicedriver.ko"

def run_command(command):
    """Run a shell command and update the terminal output."""
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    
    # Read the output in real-time
    for line in iter(process.stdout.readline, ''):
        update_terminal(line)
    for line in iter(process.stderr.readline, ''):
        update_terminal(line)

    process.stdout.close()
    process.stderr.close()
    process.wait()

def update_terminal(text):
    """Update the terminal display in the Tkinter window."""
    terminal.insert(tk.END, text)
    terminal.see(tk.END)  # Auto-scroll to latest output

def build_driver():
    """Runs `make` to compile the driver."""
    update_terminal("\n[INFO] Building driver...\n")
    threading.Thread(target=run_command, args=(f"make -C {DRIVER_DIR}",), daemon=True).start()

def clean_build():
    """Runs `make clean` to remove compiled files."""
    update_terminal("\n[INFO] Cleaning build...\n")
    threading.Thread(target=run_command, args=(f"make -C {DRIVER_DIR} clean",), daemon=True).start()

def check_module():
    """Check if the module is loaded."""
    result = subprocess.run("lsmod", capture_output=True, text=True, shell=True)
    return MODULE_NAME in result.stdout

def load_module():
    """Loads the device driver module if not loaded."""
    if not check_module():
        update_terminal("\n[INFO] Loading driver...\n")
        threading.Thread(target=run_command, args=(f"sudo insmod {MODULE_PATH}",), daemon=True).start()
        create_device_file()
    else:
        update_terminal("\n[INFO] Device driver already loaded.\n")

def unload_module():
    """Unloads the device driver module."""
    if check_module():
        update_terminal("\n[INFO] Unloading driver...\n")
        threading.Thread(target=run_command, args=(f"sudo rmmod {MODULE_NAME}",), daemon=True).start()
    else:
        update_terminal("\n[ERROR] Device driver is not loaded.\n")

def create_device_file():
    """Ensure the /dev/devicedriver file exists."""
    if not os.path.exists(DEVICE_PATH):
        os.system(f"sudo mknod {DEVICE_PATH} c 240 0")  # 240 is an example major number
        os.system(f"sudo chmod 666 {DEVICE_PATH}")  # Set read/write permissions

def spin_wheel():
    """Triggers the spin by writing to the device driver."""
    try:
        update_terminal("\n[INFO] Spinning the roulette wheel...\n")
        with open(DEVICE_PATH, "w") as dev:
            dev.write("1")  # Write to the device to trigger spin
        time.sleep(2)  # Wait for spin to complete
        get_result()
    except Exception as e:
        update_terminal(f"[ERROR] Failed to spin wheel: {e}\n")

def get_result():
    """Reads the winning GPIO pin from the device driver."""
    try:
        with open(DEVICE_PATH, "r") as dev:
            result = dev.read().strip()
        result_label.config(text=f"Winning GPIO Pin: {result}")
        update_terminal(f"[INFO] Winning GPIO Pin: {result}\n")
    except Exception as e:
        update_terminal(f"[ERROR] Failed to get result: {e}\n")

# Create Tkinter GUI
root = tk.Tk()
root.title("Raspberry Pi Roulette")

frame = tk.Frame(root, padx=20, pady=20)
frame.pack(pady=20)

title_label = tk.Label(frame, text="Raspberry Pi Roulette", font=("Arial", 16, "bold"))
title_label.pack()

# Buttons
build_button = tk.Button(frame, text="Build Driver", font=("Arial", 12), command=build_driver)
build_button.pack(pady=5)

clean_button = tk.Button(frame, text="Clean Build", font=("Arial", 12), command=clean_build)
clean_button.pack(pady=5)

load_button = tk.Button(frame, text="Load Driver", font=("Arial", 12), command=load_module)
load_button.pack(pady=5)

spin_button = tk.Button(frame, text="Spin", font=("Arial", 14), command=spin_wheel)
spin_button.pack(pady=10)

result_label = tk.Label(frame, text="Press 'Spin' to play!", font=("Arial", 14))
result_label.pack()

unload_button = tk.Button(frame, text="Unload Driver", font=("Arial", 12), command=unload_module)
unload_button.pack(pady=5)

# Terminal Output Display
terminal_label = tk.Label(frame, text="Terminal Output:", font=("Arial", 12))
terminal_label.pack(pady=5)

terminal = scrolledtext.ScrolledText(frame, width=60, height=15, font=("Courier", 10))
terminal.pack()

# Run the Tkinter event loop
root.mainloop()
import tkinter as tk
from tkinter import messagebox
import time

def load_driver():
    """Placeholder function for loading the device driver."""
    messagebox.showinfo("Info", "Loading device driver... (Not implemented yet)")

def unload_driver():
    """Placeholder function for unloading the device driver."""
    messagebox.showinfo("Info", "Unloading device driver... (Not implemented yet)")

def spin_wheel():
    """Placeholder function for spinning the roulette wheel."""
    spin_button.config(state=tk.DISABLED)  # Disable button while spinning
    result_label.config(text="Spinning... 🎰")
    root.update_idletasks()
    
    time.sleep(2)  # Simulated delay
    
    # Fake result for now
    winning_pin = "GPIO 534"  
    result_label.config(text=f"Winning GPIO Pin: {winning_pin}")
    
    spin_button.config(state=tk.NORMAL)  # Re-enable button

# Create Tkinter window
root = tk.Tk()
root.title("Raspberry Pi Roulette")

frame = tk.Frame(root, padx=20, pady=20)
frame.pack(pady=20)

title_label = tk.Label(frame, text="Raspberry Pi Roulette", font=("Arial", 16, "bold"))
title_label.pack()

# Buttons
load_button = tk.Button(frame, text="Load Driver", font=("Arial", 12), command=load_driver)
load_button.pack(pady=5)

spin_button = tk.Button(frame, text="Spin", font=("Arial", 14), command=spin_wheel)
spin_button.pack(pady=10)

result_label = tk.Label(frame, text="Press 'Spin' to play!", font=("Arial", 14))
result_label.pack()

unload_button = tk.Button(frame, text="Unload Driver", font=("Arial", 12), command=unload_driver)
unload_button.pack(pady=5)

# Run the Tkinter event loop
root.mainloop()
