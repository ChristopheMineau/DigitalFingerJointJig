# DigitalFingerJointJig
Arduino code and other files for my Digital Finger Joint Jig woodworking machine.  

# What it is all about
This project is both a hardware and Software project.  
It is related to woodworking tooling.  
The final goal is to accurately make finger joints of any width, with any blade width.  
(If this sounds weird to your ears so far ... you're free to pass your way ...)  

The project is fully described on this post on the tooling encyclopaedia homemadetools.net :  
http://www.homemadetools.net/forum/new-kind-digital-finger-joint-jig-63733#post99890

# About Me
This is personnal work from Christophe Mineau, I am an amateur craftsman and maker.  
I am in the mean time a professional software designer.  
Most of my craft work is published on my Website : www.labellenote.fr  
Most of my tooling work is also published on HomeMadeTools.net : http://www.homemadetools.net/builder/Christophe+Mineau   

# License
This work can be shared under the respect of the Creative Commons License: CC BY-NC-SA 4.0  
[![License: CC BY-NC-SA 4.0](https://img.shields.io/badge/License-CC%20BY--NC--SA%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-sa/4.0/)  

# Components
Everything is home build, using the following purchased components :  
## Arduino board :
Nano V3 is just fine and tiny  
## IR sensors
These are FC-51 IR sensors, same as described here :  
http://gilles.thebault.free.fr/spip.php?article37  
I have un-soldered the LEDS and made a black plastic mount for my rotary encoder, in order to get them at the right spacing.  
I re-connected the mounted leds to the sensor via short wires.  

These happened to be quite difficult to use eventually, generating a lot of bouncing.  
My first version using interrupts was practically un-usable, and I had to code this final version using a polling technique 
and a strong anti-bouncing algorythm.  
I am thinking now that I should have gone the magnetic/hall effect sensor way ...  

# Known issues
In the shop environment (proximity of the table saw motor), I have some time to time some erratic behaviors,
thought this is quite rare.  

# TODO
Redo the rotary sensor using hall effect sensors.  


