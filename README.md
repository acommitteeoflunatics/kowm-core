## kowm
kowm ( :speaker: /kōm/ ) never wanted to be anything more when she grew up than the 
only X window manager that even I would take home to Mom. kowm has decided 
that 'extensible simplicity' is the wave of the future; and we tend to agree.

## kowm-core
kowm-core ( :speaker: /kōm-kôr/ ) is made up of six modular desktop libraries; five to handle 
the basic features that make for a beautifully simple window manager while the 
sixth is a framework for building, attaching, and manipulating new modular 
extensions. Since speed is of the essence here, we're planning on writing kowm-core in C++.

## Requirements
Any kowm installation will require, at a bare minimum, the first six 'core' 
libraries; other requirments will be made known closer to initial release.

## Libraries

-	:capricorn: caprica provides low-level constructs shared by all 'windows'; including the root window.
-	:gemini: gemoni provide basic workspace management and functionality.
-	:leo: leonis provides the graphics subsystem from which all graphics toolkit modules can hook into and manipulate objects.
-	:sagittarius: sagitara provides a basic module structure, attachment points and shared asset management funtions.
-	:pisces: piscera provides automatic menu generation of the system menu, the desktop menu and the application menus.
-	:virgo: virgon provides all remaining mid- and high-level 'window' management

All core libraries are to be coded in C++ unless a compelling case can be 
made against it.

============
Copyright :copyright: 2009, 2010, 2011, 2012, 2013, Jerry W Jackson
All rights reserved.
