This is an OpenGL/C++ OpenStreetMap .osm XML parser and visualizer.

### Libraries are provided for ease of use, although I don't think that's strictly legal/allowed
oh well...

# Usage

After starting the program, write the input file's name into the console. (If a .cache with the same name exists, it is loaded automatically.)

Use with [Geofabrik](https://download.geofabrik.de/) .osm XML files.

Regular exports from OpenStreetMap won't work (minimal modification needed).

## Controls

* __*Pan*__ with the mouse.
* __*Zoom*__ with the mousewheel or 'q' and 'e'
* __*Load things at the center of the screen*__ with 'g'

# Caching

If a .osm file is input, the program creates a *.osm.cache *text* file, with minified information.

When the cache is being generated, roughly 2 * .osm memory is needed.

This cache is about 1/4th the size of a regular xml, and loads ~10x faster.

(could be changed to simple binary data dump)

# Load times

The console window should provide info about the opened file.

Load time is proportional to input.

For reference: an average desktop loads hungary from cache (0.5G) in ~250s.

# Processed Data / Visuals

The 'ways' from the OSM file are drawn as lines.

The lat/lon coordinates are preserved in the cache, but projected using Mercator Projection for displaying.

The 'nodes' are substituted into the ways' node references as their coordinates.
This way only the ways are stored, but currently only they are relevant anyway.

The *type* of a way determines what color it is.
The exact colors are in RGB in the source.

The 'relations' are discarded (should be used later for large lakes, etc.)

# Screenshots

![alt](/Screenshot_1.png)

![alt](/Screenshot_2.png)

![alt](/Screenshot_3.png)

![alt](/Screenshot_4.png)