import os

import numpy as np
from sklearn.model_selection import train_test_split


# STEP1: The function returns the base folder path and all dataset file names
def get_per_subject_file(subject, no_gesture):
    base_path = os.path.join(os.path.dirname(__file__), "..", "data", "raw", "subject{}".format(subject))
    data_file = os.listdir(base_path)
    gesture = {}
    for i in range(no_gesture):
        gesture[str(i)] = [file for file in data_file if file.endswith("_G{}.npy".format(i))]
    return base_path, gesture


# Not in the labmate's file -- his .mat files already have channels as separate columns.
# Our .npy files mix both sensors into one column, tagged by a sensor-ID, so we need
# this extra function to split them apart before anything else can happen.
def load_trial(path):
    data = np.load(path)
    s1 = data[data[:, 0] == 1][:, 2]
    s2 = data[data[:, 0] == 2][:, 2]
    n = min(len(s1), len(s2))
    return np.stack([s1[:n], s2[:n]], axis=0)  # (2 channels, T samples)


# STEP2: Concatenate all gesture together and include respective label as dictionary key
def get_data_per_gesture(subject, no_gesture):
    path, filename = get_per_subject_file(subject, no_gesture)
    gesture = {}

    for i in filename:

        for inx, j in enumerate(filename[i]):  # load individual file
            data = load_trial(os.path.join(path, j))

            if inx == 0:
                stack_data = data
            else:
                stack_data = np.column_stack((stack_data, data))

        gesture[str(i)] = stack_data

    return gesture  # (channels, samples_per_gesture)


# Not in the labmate's file. His pre_processing() does notch + bandpass filtering
# because the Myo armband gives raw, unfiltered EMG. The MyoWare sensor already
# filters and rectifies the signal in its own hardware before it reaches our ESP32,
# so running his filters again would be redundant. We keep this function only to
# do the normalization part of what his pre_processing() did.
#
# History of this function, and why it looks like this now:
#   v1: divided by np.max(np.abs(data)) of a whole per-gesture block. This computed
#       a DIFFERENT scale for REST vs CLOSE vs OPEN (since each gesture's own max
#       differs), which quietly baked the class label into the normalized numbers --
#       a form of data leakage. That's very likely why early accuracy looked
#       suspiciously good (97%+).
#   v2: divided by a fixed ADC_MAX=4095 instead, to remove that leakage. But this
#       exposed a second problem: all 3 classes sit around the same ~1843 baseline
#       (the sensor's resting circuit voltage, not a real signal) -- dividing by a
#       big fixed number just shrinks everything including that shared baseline,
#       burying the actual class-relevant fluctuation under a large constant term.
#       Accuracy collapsed to ~33% (chance, for 3 classes).
#   v3 (this version): remove each window's own average first (its baseline),
#       *then* scale what's left by the fixed ADC range. This keeps only the
#       fluctuation -- which is where the real REST vs CLOSE vs OPEN difference
#       actually lives -- and is legitimate for real-time use, since a window's
#       own average is always something you can compute from that window alone,
#       with no knowledge of its label or any other window.
ADC_MAX = 4095.0


def pre_processing(window):
    centered = window - window.mean(axis=-1, keepdims=True)
    return centered / ADC_MAX


# Segment the data by using sliding window -- identical to the labmate's version
def window_with_overlap(data, sampling_frequency=200, window_time=200, overlap=50, no_channel=2):
    samples = int(sampling_frequency * (window_time / 1000))
    num_overlap = int(samples * (overlap / 100))

    num_overlap_samples = samples - num_overlap
    idx = [i for i in range(samples, data.shape[1], num_overlap_samples)]  # data = (channel, samples)

    data_matrix = np.zeros([len(idx), no_channel, samples])

    for i, end in enumerate(idx):
        start = end - samples

        if end <= data.shape[1]:
            data_matrix[i] = data[0:no_channel, start:end]

    return data_matrix


# Identical to the labmate's version
def create_label(data, inx):
    label = np.zeros([data.shape[0], 1])
    label.fill(int(inx))
    return label


def get_data_subject_specific(subject, no_gesture, fs, window_time, overlap, no_channel):
    dict_gestures = get_data_per_gesture(subject, no_gesture)

    for idx, inx in enumerate(dict_gestures):
        # Window first, THEN normalize each window on its own -- this matches
        # exactly what happens live on the FPGA, where you only ever have one
        # window in hand and normalize it by itself, not by a whole gesture's
        # worth of past+future data.
        win_data = window_with_overlap(data=dict_gestures[inx], sampling_frequency=fs, window_time=window_time,
                                        overlap=overlap, no_channel=no_channel)
        win_data = pre_processing(win_data)

        label = create_label(win_data, inx)

        if idx == 0:
            data_stack = win_data
            label_stack = label

        else:
            data_stack = np.row_stack((data_stack, win_data))
            label_stack = np.row_stack((label_stack, label))

    X, y = shuffle_data(data_stack, label_stack)
    return X, y


# Identical to the labmate's version
def shuffle_data(data, label):
    idx = np.random.permutation(len(data))
    x, y = data[idx], label[idx]
    return x, y


# Identical to the labmate's version (keeping his spelling of "spilt" for consistency)
def spilt_data(data, label, ratio):
    X_train, X_test, y_train, y_test = train_test_split(data, label, test_size=ratio, random_state=42)
    return X_train, y_train, X_test, y_test
