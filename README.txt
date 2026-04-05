# NetworkSimulator
For my graduate project at RPI, I am researching network simulation and coding a computer application to run these algorithms. This will be coded in C++ using the Qt library. The main idea is to allow for users to create their own networks and run/visualize complex algorithms on their graphs. 

When first running:
scan for kits in CTRL + Shift + P
qt build


current TODO:
= fix backend according to pdf from prof, should help with larger datasets
= fix backgound dynamic sizing, doesnt work with large circles
= add node contraction, based visually, close nodes get contracted, or based edge wise, connected nodes get contracted where 
  highest X% of degree nodes get contracted to k-hop away from node
= have edges lock to certain angles, this should be a mode in view settings
= add grid to background, set to px length
= add times to algorithms




MOSTLY FIXED:
= account for multi and self edges /// temp fix, doesnt allow right now, need to allow them in future
= visualization algorithms, want to add more

FIXED:
= right click on node does not work properly, cant detect that its on node /// FIXED
= fix zoom to be scrolling
= fix scroll button pan
= fix node creation on right click, not in right spot
= fix reset zoom to be around the graph, not just a set locaation
= on edge creation make the edge follow the cursor
= fix delete for multiple selected nodes
= select and delete adges
= make a way to see the node and graph info at bottom of screen
= fix top menu for adding nodes and edges
= make a algorithm section on right side
= fix adding edge button, maybe a ad edge mode? Didnt not make add edge mode
= fix edge labels /// mostly fixed, want to add option to remove labels
= allow for node weights


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


vizualization algorithm:
Spring-Electric model suggested by Hu (2005, The Mathematica Journal, http://yifanhu.net/PUB/graph_draw_small.pdf)

how to load/save networks:
https://dshizuka.github.io/networkanalysis/02_dataformats.html

find lots of networks to load up:
https://networkrepository.com/


current stats:
large graph (V=1686, E=4623) 
    laod-time: 5s, 1.9s
    Spiral: 17s, 36s
    circular: 32s, 54s
    BFS(src=5): 9ms, 10ms
    DFS(src=5): 7.5ms, 9ms
    components: 10ms, 14ms


Medium graph (V=445, E=1335)
    load-time: 2.2s, 500ms
    spiral: 500ms, 900ms, 1.5s, 1.6s
    circular: 2s, 1.1s
    BFS(src=166): 500us, 305us
    DFS(src=166): 400us, 355us
    components: 4.85ms
    spiral: 500ms




post backend change stats:
large graph (V=1686, E=4623) 
    Spiral: 17s
    circular: 

Medium graph (V=445, E=1335)
    spiral: 500ms

    
