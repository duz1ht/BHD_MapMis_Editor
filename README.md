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
* `resources/BHD/items_lists/otheritems.js` lists all the items that aren’t in `items.def` by default and also uses new variables, like graphic to indicate which 3D `.glb` file will be used for the item.
* `resources/BHD/items_lists/weapon.def` provides the weapon catalog used to display loadout names.
* Today, when loading a map, the editor moves the camera to a spot near the center of the inserted items. This could be optimized to keep the camera from being positioned so high up in the sky.

All 3D .glb items should be in the resources/BHD/3d_items folder, and you should download the items here: https://drive.google.com/file/d/1Ra4pI8aTDwG5vO3h0fLZYzqPM6YveKQH/view?usp=sharing
How to load a map: https://youtu.be/opW0PqfdUr4

## Keep working on it.
Feel free to continue what’s already been done.
Use AI to better understand the index.html file since it’s a simple file, to see what is and isn’t implemented.
If you have any questions, feel free to contact me on Discord at @duz1ht.

## Contributors so far
biggy, Demonic, dataspiller, Scott (NovaHQ), AngelExalted
