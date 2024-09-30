###############################################################################
###
#  Brief: 	Reads data from a log file, filters all non-existent properties,
#           and saves it as a new log file.
#
#  Created:   30.09.2024
#  Author: 	Yurii Shenbor
#
###############################################################################
import sys

if (len(sys.argv) != 2):
    print("Usage: %s <File>\n", sys.argv[0])
    exit()

input_file = str(sys.argv[1]) + '.log'
output_file = str(sys.argv[1]) + '_filterred.log'
target_text = 'no such property'

# Open the input file in read mode
with open(input_file, 'r') as infile:
    # Read all lines from the file
    lines = infile.readlines()

# Filter out lines containing the target text
filtered_lines = [line for line in lines if target_text not in line]

# Write the filtered lines to the output file
with open(output_file, 'w') as outfile:
    outfile.writelines(filtered_lines)

print(f"Lines containing '{target_text}' have been removed and saved to {output_file}")
