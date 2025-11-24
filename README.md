# GAW_Speedometer
This project is built to be able to measure model train speeds. 
The core is an Arduino nano. The output is going to an LCD 16x02 display. 
Two infrared sensors are used to pick up trains rolling by and measuring 
the milliseconds it takes from one to the other sensor. 
It works bi-directional.
A measurement stays available for 5 seconds.
After that the system reverts to automatic bi-directional measurement mode.

## Settings
In version 1.2, a settings menu has been built in using the serial interface.
This menu can be reached by pressing a button.
It looks like this:


![main menu](./gfx/main-menu.png)


## Scale
In model railroading the scale of course is important for the speed calculation. 
The default scale in this program is N-scale which is 1:160. 
This default scale for the measurements can be altered to in total 8 NMRA scales.
The scale selection menu looks as follows:


![scale selection menu](./gfx/scale_select_menu.png)


## Sensors
The device works with two IR sensors. These have a certain distance, and the program has to be aware of that.
To be able to set the distance of the sensors we can activate the second menu entry (2).
There we can enter the distance in mm between the sensors.
That looks like this:



![sensor distance entry](./gfx/sensor_distance_entry.png)



## Operation
The following functions are available.
See also this state diagram:

![scale selection menu](./gfx/StateDiagram.png)


### Measurement
A measurement starts when one of the IR sensors is activated by a passing train.
The appropriate (left or right) LED will be switched on to indicate the start of the measurement.
The device now waits for the other sensor. 
It then measures the time it took for the train to reach the other sensor. 
As soon as the scond sensor is activated the scale-speed is calculated and displayed.


### Reset
The Reset button performs a hard reset of the Arduino,  creating a situation resembling a power off / power on.

## Schematic
A [schematic](./schematic/GAW_Speedometer_schematic_v1.1.png) is available to make your own.
