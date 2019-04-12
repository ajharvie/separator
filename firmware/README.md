Arduino sketch written for a Teensy 3.2 for the automated separator. To flash to a teensy, first install arduino and Teensyduino (https://www.pjrc.com/teensy/td_download.html) and follow the instructions on the teensyduino documentation.

Refer to the paper's supporting information for instructions on how to wire up the microcontroller.

The firmware will also work on most other arduino boards with minor modifications (although the board won't fit in the 3D printed mount!) - just select the pins you're using for the servo and optical sensors by changing the values in the preprocessor definitions. 
