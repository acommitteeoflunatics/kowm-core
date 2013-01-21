## kowm
kowm ( /kōm/ ) never wanted to be anything more when she grew up than the 
only X window manager that even I would take home to Mom. kowm has decided 
that 'extensible simplicity' is the wave of the future; and we tend to agree.

## kowm-core
kowm-core is made up of six modular desktop libraries; five handle the basic features that make for a beautifully simple window manager and the sixth is a framework for building, attaching, and manipulating new modular extensions. Since speed is of the essence here, we're planning on writing kowm-core in C++.
## Requirements
Any kowm installation will require, at a bare minimum, the first 6 'core' 
libraries:

-	`caprica` handles low-level window management 
-	`gemoni` handles low-level network management 
-	`leonis` handles low-level graphics
-	`sagitara` is a low-level framework for building and connecting modules
-	`piscera` handles low-level system menuing
-	`virgon` handles low-level system translation 

All core libraries are to be coded in C++ unless a compelling case can be 
made against it.

============
Copyright (c) 2009, 2010, 2011, 2012, 2013, Jerry W Jackson
All rights reserved.
