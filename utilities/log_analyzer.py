###############################################################################
###
#  Brief: 	Reads data from a port log file, processes it, and saves it as an image.
#
#  Created:   29.09.2024
#  Author: 	Yurii Shenbor
#
###############################################################################
import matplotlib.pyplot as plot
import json
import sys

if (len(sys.argv) != 2):
    print("Usage: %s <Port>\n", sys.argv[0])
    exit()

input_file_name = str(sys.argv[1]) + '.log'
output_file_name = str(sys.argv[1]) + '.png'

time_list = []
value_list = []

print("Processing log file:", input_file_name)
with open(input_file_name, 'r') as file:
    for line in file:
        data_point = json.loads(line.strip())
        time_list.append(int(data_point['timestamp']))
        value_list.append(float(data_point['data']))

# Calculate amplitude
amplitude = 'Amplitude(V): ' + str(max(value_list) - min(value_list))

# Calculate frequency
period = 0
for i in range(1, len(time_list)):
    period += time_list[i] - time_list[i - 1]
period //= len(time_list)
frequency = 'Frequency(Hz):' + str(round(1000 / period, 2))

# Convert timestamp to time scale
start_time = time_list[0]
for i in range(0, len(time_list)):
    time_list[i] = (time_list[i] - start_time)

# Create a figure and a set of subplots
fig, ax = plot.subplots(1)
ax.plot(time_list, value_list)
ax.set_ylabel('Data')
ax.set_xlabel('Time(ms)')
ax.text(0.1, -0.4, frequency, horizontalalignment='center',verticalalignment='center', transform=ax.transAxes)
ax.text(0.1, -0.5, amplitude, horizontalalignment='center',verticalalignment='center', transform=ax.transAxes)
ax.tick_params(axis='x', rotation=45)
ax.grid(True)

# Save the figure as an image
plot.tight_layout()
plot.savefig(output_file_name)
print("Result saved:", output_file_name)