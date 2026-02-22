# NetworkSimulator
For my graduate project at RPI, I am researching network simulation and coding a computer application to run these algorithms. This will be coded in C++ using the Qt library. The main idea is to allow for users to create their own networks and run/visualize complex algorithms on their graphs. 

When first running:
scan for kits in CTRL + Shift + P
qt build


current TODO:
= make a algorithm section on right side
= make a way to see the node and graph info at bottom of screen
= fix top menu for adding nodes and edges
= account for multi and self edges
= on edge creation make the edge follow the cursor
= fix delete for multiple selected nodes
= select and delete adges


MOSTLY FIXED:
= fix edge labels /// mostly fixed, want to add option to remove labels

FIXED:
= right click on node does not work properly, cant detect that its on node /// FIXED
= fix zoom to be scrolling
= fix scroll button pan
= fix node creation on right click, not in right spot
= fix reset zoom to be around the graph, not just a set locaation


Network simulator
C++ for backend with Qt (with QGraphicsView/Qt Quick), used to make networks and gui frontend
•	Want to use network/graph definitions, maybe make a definition page
•	Want to create add scripting to that the user can create their own algorithms
•	Want to try and make application specific files to save to (.ns ???)
•	Want to be able to see statistics and visualization of the algorithm
•	Upload this to git hub to keep track of versions
•	Currently this will be a computer app with a backend and a front end, I could also add a web frontend if I make the backend compatible and loosely coupled with the front end.

Questions:
•	Finish by April 1st? get most of what I want to do done, there is a lot that I could add to this, I could go on forever
•	AI usage?



intro -motivation, what I hoep to achieve
review of existing work- important, not just starting from scratch, look for good papers, not old and lots of cytations,
what I propose - what my algorithm or what algorihtm I mprove upon
my implimentation
experimental results - good or bad result, report it
analysis of results - steps for further development
summary
citations - keep all urls that help

why I used the tools I did