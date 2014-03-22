TODO LIST
======

Open Source Computed Tomography (CT) Scanner
http://www.tricorderproject.org/openct
Copyright (C) 2014  Peter Jansen and the Tricorder project 
peter@tricorderproject.org

One of the most common questions an organizer of an open source project receives is, "How can I help?".  This TODO list outlines some of the near-term contributions that can be made to the project in a variety of categories, varying from simple to challenging. 

# Structural and Mechanical

- *End-stops:* Add end-stops ("limit switches") to each of the two linear axes, table axis, and rotational axis. 

- *Cabling:* Find a better solution to cable routing.  Cable routing is currently an issue, and there are a number of cables and signals that have to move from the rotating bore to the controller/arduino shield on the stationary base.  The rotational axis has to have at least 180° of rotation, meaning that at one extreme it has a lot of slack, and at the other it has zero slack.  

- *Cabling, Part 2:* Is cabling the answer?  Should we turn the bore into a giant slip ring with two lines (+5V, GND) so that it can freely rotate 360°, and wirelessly communicate between the bore and base using (for example) bluetooth? 

# Electronics

- *Arduino Shield:* Lay out an Arduino shield, for the latest Uno revision, that includes: (1) 4 Pololu stepper controllers (3 @ 5V, one selectable between 5V and 12V for the larger rotary axis stepper), (2) an SD/uSD socket for data storage, and (3) Headers/connectors for the end-stops, and an I2C accelerometer for sensing the rotational angle of the bore.  This should be compact enough to fit and mount inside one side of the base, and expose any required ports or connectors on the Arduino/Shield using clean cutouts in the back of the base. 

- *Rotational Sensing:* Currently the plan is to use an I2C accelerometer mounted in the bore as an absolute rotational sensor to sense the current angle of the bore.  Create the board for this, and add the supporting code into to the Arduino sketch.  A sensible place for this might be a tiny board that securely mounts on the back of the carriages and combines an accelerometer and two limit switches.  

# Source and Detector

- *Parallel Detector:* The current source/detector system is basically a working model of the first generation of CT scanner, that makes use of a single source and detector that are linearly scanned across the sample.  Modern systems speed this up dramatically by parallelizing the problem and using an large array of detectors in place of a single detector -- removing the need for linear scanning stages inside the bore, and reducing scan time by measuring all points for a given angle simultaneously.  This is a fairly large undertaking, and would likely require developing a custom parallel detector array with tens of detectors as inexpensively as possible.  

- *Alternate Sources/Detectors:* Using your advanced knowledge of optics, develop alternate source/detector pairs that examine different, safer wavelengths that objects of interest tend to be semi-transparent at.  (possible examples: Terahertz? Radio waves? Infrared?)

