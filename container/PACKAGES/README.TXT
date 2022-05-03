Toomey Nov 2021

Basically we create a folder for each container with files required for that level of building.
This folder is copied to /tmp during the container build and contains static files to place
on the container.
Note: Changing a file here means you need to rebuild that container and ones after it,
      which can be a bit annoying if you're changing code, etc.  Another way is to run
      the container and work directly on it then save when finished changing.

Containers:

MRMSBASE           -- Base with all third party installed used by MRMS, HMET, RAPIO
--> RAPIO          -- RAPIO development container on top of MRMSBASE

UNCRUSTIFY         -- Container running version of uncrustify currently using to
                      pretty format code with on check-ins.  Different versions, etc.
                      can do slightly different output so this standardizes it on
                      different OS and versions.
                   -- Building this container allows you to use the check and format
                      scripts in the container folder to pretty print on any OS
