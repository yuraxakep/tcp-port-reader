###############################################################################
###
#  Brief: 	Reads data from the log file of the client app, calculates amplitude,
#           frequency and build graph. Checks for the correct timing.
#
#  Created:   01.10.2024
#  Author: 	Yurii Shenbor
#
###############################################################################
import matplotlib.pyplot as plot
import json
import sys

if (len(sys.argv) != 3):
    print("Usage: %s <client log file> <timming>\n", sys.argv[0])
    exit()

input_file_name = str(sys.argv[1]) + '.log'
output_file_name = str(sys.argv[1]) + '.png'
timming = int(sys.argv[2])

time_list = []
value_list = [[],[],[]]

print("Processing log file:", input_file_name)
try:
    with open(input_file_name, 'r') as file:
        for line in file:
            data_point = json.loads(line.strip())
            time_list.append(int(data_point['timestamp']))
            for i in range(3):
                if data_point['out' + str(i + 1)] != '--':
                    value_list[i].append(float(data_point['out' + str(i + 1)]))
                else:
                    value_list[i].append(None)

except FileNotFoundError:
    print(f"Error: The file {input_file_name} does not exist.")
except json.decoder.JSONDecodeError:
     print(f"JSON Decode Error: skipping line...")
except Exception as e:
    print(f"An error occurred: {e}")

# Replace '--' witch the closest valid values
last_valid = [None, None, None]
for i in range(3):
    for j in range(len(value_list[i])):
        if value_list[i][j] == None and last_valid[i] == None:
            n = j
            while n < len(value_list[i]) and value_list[i][n] == None:
                n += 1
            last_valid[i] = value_list[i][n]
            value_list[i][j] = last_valid[i]
        elif value_list[i][j] == None:
            value_list[i][j] = last_valid[i]
        else:
            last_valid[i] = value_list[i][j]

# Calculate amplitude
max_val = []
min_val = []
amplitude = []
for i in range(3):
    max_val.append(max(value_list[i]))
    min_val.append(min(value_list[i]))
    if max_val[i] > -min_val[i]:
        amp_val = max_val[i]
    else:
        amp_val = -min_val[i]
    amplitude.append('Amplitude(V): ' + str(amp_val))

# Calculate frequency
frequency = []
for j in range(3):
    start = None
    period = 0
    samples = 0
    for i in range(0, len(value_list[j])):
        # If the lower point of the wave is found, save the start time
        if start == None and (value_list[j][i] == min_val[j] or 
            (i < (len(value_list[j]) - 2) and value_list[j][i + 1] > value_list[j][i])):
            start = time_list[i]
        # If the upper point of the wave is found, calculate time period
        if start != None and (value_list[j][i] == max_val[j] or 
            (i < (len(value_list[j]) - 2) and value_list[j][i + 1] < value_list[j][i])):
            period += (time_list[i] - start)
            start = None
            samples += 1

    if samples != 0:
        period //= samples
    period *= 2
    frequency.append('Frequency(Hz):' + str(round(1000 / period, 2)))

# Convert timestamp to time scale
start_time = time_list[0]
for i in range(0, len(time_list)):
    time_list[i] = (time_list[i] - start_time)

# Test for 
failed = False
for i in range(0, len(time_list) - 1):
    if (time_list[i] + timming) != time_list[i + 1]:
        failed = True
        break
if failed:
    print('Test for the correct timing: failed')
else:
    print('Test for the correct timing: success')

# Create a figure and a set of subplots
fig, ax = plot.subplots(3, 1, figsize=(10, 15))
for j in range(3):
    ax[j].plot(time_list, value_list[j])
    ax[j].set_ylabel('Out' + str(j + 1))
    ax[j].set_xlabel('Time(ms)')
    ax[j].text(0.1, -0.3, frequency[j], horizontalalignment='center',verticalalignment='center', transform=ax[j].transAxes)
    ax[j].text(0.1, -0.4, amplitude[j], horizontalalignment='center',verticalalignment='center', transform=ax[j].transAxes)
    ax[j].tick_params(axis='x', rotation=45)
    ax[j].grid(True)

# Save the figure as an image
plot.tight_layout()
plot.savefig(output_file_name)
print("Result saved:", output_file_name)