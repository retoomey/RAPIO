Toomey July 2021

Build a docker container for cross-compiling a RAPIO algorithm.
Requirements: Docker
Differences: This uses RPMS for ALL third party, except for g2clib which has been upgraded from source.
TODO: It's quite possible to svn checkout build MRMS/HMET on top of this container,
      probably I can see trying that in future.  It will probably take some tweaking.
NOTE: FAM input probably doesn't work...polling should though.  New notification methods should be in the works.
NOTE: The building user id/group is used internally to the container when running.  So if you build as fred
      and move to another computer it tries to run as that user.

Watch the YouTube tutorial for details on this stuff if it's confusing.
It can be challenging at first if you aren't familar with containers.

https://www.youtube.com/watch?v=tWrU2KuBufc


If you understand containers you can do lots of things like multiple folders, etc.
by modifying going crazy with the provided example scripts.
This is just me trying to set you up with the quickest/easiest way of making
a new algorithm. 

1. First successfully build a rapio docker image:

./build_container.sh

You can now type 'docker images' and look for the rapio container.  Lots of things
can go wrong here, usually this means IT needs to set up docker correctly for you.

2. Set up input, output and algorithm development folders on the host:

  vim config.sh // or your favorite editor.  
                   I put some variables here for other scripts.
                   You could also put these variables inside another script, etc.

  a. The input folder will be where you put test cases, etc.
     By default I make a $HOME/DATAIN.  You can stick archives in there.
     Here we map your host folder to /INPUT on container
     Advanced users can map multiple folders, etc. using docker options.
  b. The output folder will be where you want the algorithm to write to.
     Here we map your host folder to /OUTPUT on container
  c. The algorithm folder will be the folder on your host to contain your algorithms.

3. Use ./run_container.sh to start up new container
   If successful, you should have a /INPUT /OUTPUT and /MYALGS folders on the container.

4. Doxygen/dot is on the container.  If you run the container and go to /BUILD/MYALGS
 you can run the makedocs.sh script to generate doxygen documentation on the host.
