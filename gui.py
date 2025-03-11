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
