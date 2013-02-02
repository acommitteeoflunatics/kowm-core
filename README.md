## kowm
kowm ( :speaker: /kōm/ ) never wanted to be anything more when she grew up than the 
only X window manager that even I would take home to Mom. kowm has decided 
that 'extensible simplicity' is the wave of the future; and we tend to agree.

## kowm-core
kowm-core ( :speaker: /kōm-kôr/ ) is made up of eight modular desktop libraries; seven to handle 
the basic features that make for a beautifully simple window manager while the 
sixth is a framework for building, attaching, and manipulating new modular 
extensions. Since execution speed is of the essence here, we're planning on writing kowm-core in C++.

## Requirements
Any kowm installation will require, at a bare minimum, the first eight 'core' 
libraries; other requirments will be made known closer to initial release.

## Libraries

-   `baseAnnex` provides a basic module infrastructure and shared asset management funtions.
-   `baseConsumer` provides stubs and mount points for baseAnnex modules.
-   `baseDecor` provides a basic graphics subsystem from which all graphics toolkit modules can hook into and manipulate objects.
- 	`baseFare` provides automatic menu generation of the system menu, the desktop menu and the application menus.
-   `baseFlaps` provides a robust tabbing mechanism
-   `baseGear` provides simple constructs shared by all 'objects'; e.g. Spaces, Flaps and 'windows' (including the root window).
-   `baseOverseer` provides advanced maniuplation of 'objects'; aka 'window' management.
-   `baseSpaces` provide basic workspace management and functionality.

All core libraries are to be coded in C++ unless a compelling case can be 
made against it.

============
Copyright :copyright: 2009-2013, Jerry W Jackson
All rights reserved.
