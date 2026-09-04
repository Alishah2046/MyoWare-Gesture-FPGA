========================================================================
 MyoWare 2.0 2-Channel EMG Gesture Dataset
 Date: 2026-08-16
 SamplingRate: 200Hz (delay 5ms or 5,000us)
========================================================================

1. File Naming Convention
------------------------------------------------------------------------
Format: S{Subject_ID}_R{Repeat_No}_G{Gesture_Label}.npy
Example: S1_R1_G0.npy (Subject 1, Repeat 1, Gesture Label 0)

- Subject_ID : Subject ID (1: Hyeonmin, 2: Saehyung)
- Repeat_No  : Trial/Repeat Number (1, 2, 3...)
- Gesture    : Gesture Label (0, 1, 2)

2. Gesture Mapping
------------------------------------------------------------------------
- G0: REST  (Hand at rest / relaxed, 5 seconds)
- G1: CLOSE (Fist / Hand closed, 5 seconds)
- G2: OPEN  (Fingers extended / Hand open, 5 seconds)

3. Data Structure
------------------------------------------------------------------------
- File Format: NumPy Binary Format (.npy)
- Array Shape: [N, 4]  (N = Number of samples)
- Column Descriptions:
  * Col 0: Sensor ID (1 or 2)
  * Col 1: Timestamp (Microseconds, us)
  * Col 2: Raw EMG Value (ADC sensor value)
  * Col 3: Gesture Label (0, 1, 2)

4. Python Code Example
------------------------------------------------------------------------
import numpy as np

# Load dataset
data = np.load("S1_R1_G0.npy")

# Filter by Sensor ID
s1_data = data[data[:, 0] == 1]  # Sensor 1
s2_data = data[data[:, 0] == 2]  # Sensor 2

# Extract Raw EMG Values
s1_raw = s1_data[:, 2]
s2_raw = s2_data[:, 2]