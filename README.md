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

### Emplacement attachment anchors

The item catalog supports every `addeweap`, `addeweapG`, and `addeweapC` line in
a definition, including repeated entries and the optional four angular limits.
The referenced child item is rendered under the matching anchor in the parent
GLB. Anchor lookup is case-insensitive, trims surrounding whitespace, and uses
the first complete-name match.

For converted `.3di` assets, export each `USRP` record as a meshless glTF node
under the node that represents its `subobject_index`. Preserve its position and
direction in the node transform, and add the following `extras` metadata:

```json
{
  "bhdUserPoint": true,
  "userPointName": "ewep02",
  "userPointType": 0,
  "subobjectIndex": 3
}
```

The editor also accepts a meshless node whose `name` is the anchor name for
older GLBs. If no anchor is present, it logs a warning and attaches the child at
the parent origin. Angular limits are retained as metadata for future aiming
controls and do not change the initial attachment pose.
* `resources/BHD/items_lists/weapon.def` provides the weapon catalog used to display loadout names.
* Today, when loading a map, the editor moves the camera to a spot near the center of the inserted items. This could be optimized to keep the camera from being positioned so high up in the sky.

## How to use
Download the repository files and extract them to any folder.
Download the rest of the files from the resources folder using this link https://drive.google.com/file/d/1oQTO6digk8jT5PJ63xNG84RsM0UKE845/view?usp=sharing, then extract them into the editor folder.

Open index.html, click File > Set Editor Folder so the editor can read the files in the folder. After that, you can open any map you want.
The resources folder already has a folder called mis_examples, so you don't have to prepare any maps. You can test those.

## Keep working on it.
Feel free to continue what’s already been done.
Use AI to better understand the index.html file since it’s a simple file, to see what is and isn’t implemented.
If you have any questions, feel free to contact me on Discord at @duz1ht.

## Contributors so far
biggy, Demonic, dataspiller, Scott (NovaHQ), AngelExalted
