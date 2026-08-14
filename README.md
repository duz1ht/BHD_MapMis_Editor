This project ports the map editor from Delta Force: Black Hawk Down, released in 2003 by NovaLogic, into a simple HTML file using Three.js. We can keep it as a web app or later migrate it to something simple like Electron, or another platform that makes it easy to reuse and build on the existing work.

Todo:
- [x] 3D terrain rendering
- [x] Items conversion from .3DI to .GLB
- [x] General Information
- [X] Waypoints
- [X] Layer Names
- [X] Groups
- [ ] 90% - Item Attributes
- [X] 90% - Area Triggers
- [ ] 90% - Item context menu
- [X] Canvas context menu
- [ ] Briefing
- [x] Events
- [ ] Global Replace
- [ ] Weapon Loadouts

* `resources/BHD/items_lists/itemsDef.js` works similarly to the original `items.def`.
* `resources/BHD/items_lists/otheritems.js` lists all the items that aren’t in `items.def` by default and also uses new variables to indicate which 3D `.glb` file will be used for the item.
* `resources/BHD/items_lists/weapon.def` provides the weapon catalog used to display loadout names.
* Today, when loading a map, the editor moves the camera to a spot near the center of the inserted items. This could be optimized to keep the camera from being positioned so high up in the sky.

### Item direction arrows

Item-list definitions can include `arrow <meters>` immediately below `tint`. A positive decimal value enables a horizontal direction arrow whose tail starts at the item's pivot, whose direction follows the MIS heading convention, and whose length is exactly `<meters>` in world space. Pitch is intentionally ignored so the direction remains readable against the terrain. Arrows are always white (`#FFFFFF`) and do not inherit `tint` or selection colors. The arrow follows item visibility, does not participate in picking, and is rendered even when the item's GLB is unavailable. This is editor-only visual metadata and is never serialized into a `.mis` file.

All 3D .glb items should be in the resources/BHD/3d_items folder, and you should download the items here: https://drive.google.com/file/d/1Ra4pI8aTDwG5vO3h0fLZYzqPM6YveKQH/view?usp=sharing
How to load a map: https://youtu.be/opW0PqfdUr4

## Shortcuts and menu options
WASD = Camera movement
Q/E = Move the camera up and down
Mouse scroll = Increases camera speed, like in a traditional map editor such as Unreal Engine
Speed Scalar = Changes the speed scale controlled by the mouse scroll
Ground Clamp = Prevents the camera from going below the terrain
Clearance = Adjusts the camera’s minimum height
Draw Radius = Maximum terrain render distance (helps improve FPS)
Main Area = Shows the main map area before it starts repeating infinitely
Show Sectors = Shows each terrain subdivision
Go To = Lets you jump directly to a coordinate
R = Switches the view mode between colormap, heightmap, and depth map
T = Toggles an orthographic top view at the Perspective View focus point; drag to pan and use the mouse wheel to zoom, then press T to return while preserving that focus<br>
E = Changes the item gizmo to rotation mode (yaw and pitch)
W = Changes the item gizmo to standard XYZ movement mode
Gizmo Space = Switches the transform axes between world and item-local orientation<br>
Gizmo Pivot = Uses the selection center, active item, or individual item origins<br>
Rotation Snap = Toolbar toggle that quantizes gizmo rotation; use its arrow to configure the angle increment and relative or absolute mode<br>
Snap to Floor = Context-menu command that moves selected items vertically until the bottom of their transformed geometry rests on terrain or the highest item surface below<br>
Work Grid = Uses the XY top plane; the infinite grid follows the camera while keeping a stable world origin<br>
F = Toggles item wireframe view
G = Cycles guide visibility: grid + Main Area, hidden, Main Area only, grid only<br>
H = Toggles item anchor points
I = Toggles the scene info box on the canvas<br>
C = Closes or opens the left sidebar<br>

## Keep working on it.
Feel free to continue what’s already been done.
Use AI to better understand the index.html file since it’s a simple file, to see what is and isn’t implemented.
If you have any questions, feel free to contact me on Discord at @duz1ht.

## Contributors so far
biggy, Demonic, dataspiller, Scott (NovaHQ), AngelExalted

### 3D item color precedence
GLB item colors are applied in this order: the model's base material, the item-definition `tint`, waypoint or Area Trigger colors, and finally temporary viewport modes or selection visuals. Each later layer intentionally overrides the earlier layers while it applies.
