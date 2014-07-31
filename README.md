# Arduino driver for TP-KN10 board

Remove the PLCC chip, solder wires to socket pins 13, 14 & 15 at least.

## Notes about the board itself

44 pin PLCC socket, note the funky pin order (http://i207.photobucket.com/albums/bb73/ultimateroadwarrior/44pinplcc.jpg)

Has onboard 1A 5V switching reg, accpets 7-40V input (24V nominal)

41 GND (also pin 3 on "display bus")
44 5V (also pin 1 on "display bus")

19 latch for row driver (74)595 (also pin 14 on "display bus") -> A4
17 latch for column driver 595s (all of them...) (also pin 13 on "display bus") -> A3

35 data for all of the 595s (both row and column)  (also pin 16 on "display bus") -> D11
35 clock for all of the 595s (both row and column) (also pin 15 on "display bus") -> D13


13 A for the 74138 (drives output-enables for the column drivers) -> A0
14 B for the 74138 (drives output-enables for the column drivers) -> A1
15 C for the 74138 (drives output-enables for the column drivers) -> A2

These are not on the display-bus, the output-enables are, but we'll ignore those for now.

