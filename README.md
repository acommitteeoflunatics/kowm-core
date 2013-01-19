## Introduction

Having searched for many years for a usable Linux window manager that both 
appealed to my sense of aesthetics and had at least some of the features I 
feel necessary for a 'proper' WM; I only came across one which [at least 
partially] fit the bill. `Mavosxwm` was the creation of `Martin Vollrathson` 
and hadn't been updated in any way for a long time at that point. After 
obtaining permission from him, I forked Mavosxwm to use in the creation of 
`koWm`. I will be keeping the tabbed interface, the theming mechanism, and 
most of the event-handling code; but I believe a change of language is in 
order before implementing additions and changes to the existing codebase. My 
first thoughts were to use Python as my base language; now I'm leaning towards 
something Ruby-flavored. To build this wm I have decided to, first, build a set
of six of a final twelve desktop libraries so that I may make this window 
manager as extensible as it cares to be. It will be named `koWm` [ sounds like
kohm or comb ] and will be the first application built using the `kobol` 
collection of libraries found at libs/kobol.

## kowm
`koWm`, a modular and extensible window manager for Xorg based on xcb rather than
the archaic xlib.

## Requirements
Any installation will require, at a bare minimum, the first 6 'core' kobol libraries:

-	`caprica` handles low-level window management 
-	`gemoni` handles low-level network management 
-	`leonis` handles low-level graphics
-	`sagitara` is a low-level framework for building and connecting modules
-	`piscera` handles low-level system menuing
-	`virgon` handles low-level system translation 

Core libraries are to be coded in C++ unless a compelling case can be made against it.


============
Copyright (c) 2011, Jerry W Jackson
All rights reserved.
