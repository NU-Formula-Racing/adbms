import pandas as pd
import numpy as np
from gg_plotter import gg_animation

# Brake Test
brake_df = pd.read_csv("Brake_test_CSVs/Linear Accelerometer.csv")

ax_brake = -1*brake_df["Y (m/s^2)"]/9.8
ax_brake_smoothed = ax_brake.rolling(window=3, center=True).mean()
ax_brake_smoothed = ax_brake_smoothed.ffill().bfill()

ay_brake = brake_df["X (m/s^2)"]/9.8
ay_brake_smoothed = ay_brake.rolling(window=3, center=True).mean()
ay_brake_smoothed = ay_brake_smoothed.ffill().bfill()

gg_animation(ax_brake_smoothed, ay_brake_smoothed, 20, "gg_brake.mp4")


# Auto Laps
auto_df = pd.read_csv("auto_cross_CSVs/Linear Accelerometer.csv")

ax_auto = -1*auto_df["Y (m/s^2)"]/9.8
ax_auto_smoothed = ax_auto.rolling(window=3, center=True).mean()
ax_auto_smoothed = ax_auto_smoothed.ffill().bfill()

ay_auto = auto_df["X (m/s^2)"]/9.8
ay_auto_smoothed = ay_auto.rolling(window=3, center=True).mean()
ay_auto_smoothed = ay_auto_smoothed.ffill().bfill()

gg_animation(ax_auto_smoothed, ay_auto_smoothed, 20, "gg_auto.mp4")