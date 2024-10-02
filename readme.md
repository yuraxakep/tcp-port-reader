## client1:
Reads data from TCP ports 4001 ... 4003 and prints it to standard output.

The program outputs a line every 100 milliseconds. Each line is a JSON object 
with four key-value pairs: `timestamp`, `out1`, `out2`, and `out3`.

If no value is received from the server port within the specified period of 100ms,
the value for that port is set to `"--"`. If multiple values ​​are received from
the port within 100ms, the last value will be printed.

* Algorithm:
The program establishes a connection to all three TCP ports, then enters an infinite
while loop where the current time is obtained using the `gettimeofday` function.
When the current time is equal to or greater than the target time, an if statement
is executed in which the new target time is calculated and data is read from all
three ports sequentially, starting with the slowest, the resulting data is printed
to standard output, the data can be redirected to a file if desired. 

Build and usage:
```
make client1
./client1
```

## Frequencies, amplitues and shapes:
Additional software tools were developed to measure frequencies, calculate
amplitudes, and visualize shapes. The `tcp_logger` is used to capture data from each
TCP port at its output rate and save this data to a .log file for further processing.
The `log_analyzer` is used to process the .log file produced by `tcp_logger`, it
calculates the amplitude by finding the largest value from zero to a positive or
negative direction, the frequency is calculated by finding the average time period
of the wave, the shape is visualized using the `pyplot` library. Text and image logs
from each port can be found in the `/logs` directory. 

/***********************************************************************************/
Port:               Frequency:                  Amplitude:                  Shape:
4001                0.49 Hz                     5 V                         Sine
4002                0.27 Hz                     5 V                         Triangle
4003                0.16 Hz (irregular)         5 V                         Square
/***********************************************************************************/

Ussage:
```
make tcp_logger
./tcp_logger port_number log_duration_sec (i.e. ./tcp_logger 4001 30)
python3 log_analyzer.py path_to_log_file (i.e. python3 log_analyzer.py ../logs/4001)
```

## client2:
Reads data from TCP ports 4001 ... 4003 and prints it to standard output.

The program outputs a line every 20 milliseconds. Each line is a JSON object 
with four key-value pairs: `timestamp`, `out1`, `out2`, and `out3`.

If no value is received from the server port within the specified period of 20ms,
the value for that port is set to `"--"`. If multiple values ​​are received from
the port within 20ms, the last value will be printed.

In addition to the above, the program controls the behavior of the server as follows:

* When the value on the output 3 of the server becomes greater than or equal 3.0:
    * Set the frequency of server output 1 to 1Hz.
    * Set the amplitude of server output 1 to 8000.

* When the value on the output 3 becomes lower than 3.0:
    * Set the frequency of server output 1 to 2Hz.
    * Set the amplitude of server output 1 to 4000.

* Algorithm:
The program establishes a connection to all three TCP ports and starts the UDP
server, then enters an infinite while loop where the current time is obtained using
the ```gettimeofday``` function. When the current time is equal to or greater than
the target time, an if statement is executed in which the new target time is
calculated and data is read from all three ports simultaneously using threads,
the received data from port 4003 is processed to determine if there is a need to
change the behavior of port 4001, after the data from all ports is printed to
standard output, and then the cycle repeats.

Build and usage:
```
make client2
./client2
```

## Control Protocol:
Since the control protocol for the UDP port was not fully described, additional
software tools were developed to bruteforce all possible combinations of object and
property fields. The `udp_logger` was used to perform a read operation on all
possible values ​​of the `object`, after which it was narrowed down to the values
​​1, 2, and 3, which represent ports 4001, 4002, and 4003 respectively, then a read
operation was performed using all possible `property` values ​​for these three
objects. The outputs of the above operations were saved to a file and filtered from
invalid properties using `text_filter`, resulting in a list of all valid properties
for all valid objects. Logs can be found in the `/logs` directory.

Ussage:
```
make udp_logger
./udp_logger
python3 text_filter.py file_path (i.e. python3 text_filter.py ../logs/property_ch1)
```

## Result verification:
The `timing_test` script was developed to ensure that both client programs meet the
timing requirements. Based on the log file created by the client program, it
calculates the difference between the current and previous timestamps, and if the
difference is equal to the set timeout, then the test is passed, otherwise it fails.
Another feature of the `timing_test` script is that it creates a visual representation
of the signals for all ports with a time scale, allowing for the visual verification
of the changing frequency and amplitude on port 4001 as a result of changing values on
port 4003.

Ussage:
```
python3 timing_test.py file_path timming (i.e. python3 timing_test.py ../logs/client2 20)
```