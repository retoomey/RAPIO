IO module for NSSL HMET MRMS binary files. 

These are the special binary files used by the HMET group for speed/memory reduction in certain algorithms.
They use division scaling to store floats into two bytes at the cost of data resolution.

Action list on this module:

Done:
1. Read/write mrms polar
2. Read/write mrms gridded binary lat lon grid
3. Read/write mrms gridded binary lat lon height grid
4. Implement table/.dat for product names from mrms to w2 amd missing.
5. Break up reader/writers into separate files per type to be cleaner.
  -- Testing here by just reading one with rtile and displaying in webbrowser

Todo:
6. Projection cleanup/param interface since tiles, etc. I'm just locking to height 0 for LatLonHeightGrd
7. Possibly make product name table modern XML or something or add that ability.
   Dealing with legacy vs modern at this point.
