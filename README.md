# GAW_Speedometer
This project is built to be able to measure model train speeds. 
The core is an Arduino nano. The output is going to an LCD 16x02 display. 
Two infrared sensors are used to pick up trains rolling by and measuring 
the milliseconds it takes from one to the other sensor. 
It works bi-directional.

## Scale
In model railroading the scale of course is important for the speed calculation. 
The default scale in this program is N-scale which is 1:160. 
This default scale for the measurements can be altered to in total 8 NMRA scales.

## Operation
The following functions are available.

### Start Measurement
Measurement starts when one of the IR sensors is activated by a passing train.
The appropriate (left or right) LED will be switched on to indicate the start of the measurement.
The device now waits for the other sensor. 
It now measures the time it took. 
As soon as the scond sensor is activated the scale-speed is calculated and displayed.

### Set Scale
When the device is switched on the default scale is presented. 
As long as no measurement is taking place one can use the scale button, 
to switch to the next scale in the scale table.


Available scales (from NMRA) are:

| Index | Scale name  | Factor |
|-------|-------------|--------|
| 0     | O(17)       | 45.2   |
| 1     | O, On3, On2 | 48.0   |
| 2     | Sn3, S      | 64.0   |
| 3     | |OO         | 76.0   |
| 4     | |HO         | 87.0   |
| 5     | TT          | 120.0  |
| 6     | N, Nn3      | 160.0  |
| 7     | Z           | 220.0  |

In the program, the default scale is designated by the value of ***scalePtr***, it's initial value is 6 for N-scale.

### Reset
The Reset button performs a hard reset of the Arduino, 
creating a situation resembling a power off / power on.

## Schematic
A [schematic](./schematic/GAW_Speedometer_schematic_v1.1.png) is available to make your own.
