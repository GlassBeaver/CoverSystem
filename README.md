# CoverSystem
Real-Time Dynamic Cover System for Unreal Engine 4

Used in "Severed Steel" a game by Matt Larrabee!
https://store.steampowered.com/app/1227690/Severed_Steel/

See my article: https://horugame.com/real-time-dynamic-cover-system-for-unreal-engine-4/
<br/>Or my presentation, click on the PowerPoint logo: https://horugame.com/cover-system-talk-and-slides/

```diff
- You'll need to CLONE the project! Zip downloads don't work as I'm using git-lfs for storing binary files.
```

Key features:
=================================

Data generation:
- navmesh edge-walking
- 3D object scanning
    
Data persistence via octree

Navmesh error-handling
 
Multi-threaded, asynchronous cover generation

Real-time dynamic updates via Recast events

Profiler integration

Runs on UE 4.22 - 4.26
