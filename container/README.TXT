Build various containers, see list below
Requirements: Docker or Podman

Currently for these containers I'm using RPMS for ALL third party, with the cooresponding pros and cons.
However the cmake build of WDSS2 at some point will allow toggling here I think.

NOTE: FAM actually works inside a container due to FUSEFS
NOTE: For say the RAPIO development image, the building user id/group is used internally to the container when running.  So if you build as fred and move to another computer it tries to run as that user.

Watch the YouTube tutorial for details on this stuff if it's confusing.
It can be challenging at first if you aren't familar with containers.
This video was the original pass so there are slight differences now in the scripts, etc.  The basic
ideas should still be the same

https://www.youtube.com/watch?v=tWrU2KuBufc

I've broken the containers down into layers, some of which are required depending on your needs.
Also you can try building in order if you have issues.

  A. build_mrmsbaseops.sh

     REQUIREMENTS: None
     USAGE: Base for small deployment images for just running MRMS/RAPIO code
            So basically no headers, compilers, code versioning.

  B. build_mrmsbasedev.sh 

     REQUIREMENTS: mrmsbaseops from A
     USAGE: Build a developement image.  This adds the headers, git/svn, g++, etc.
            that would be needed to actually build code.  It's obviously a lot bigger.
            Currently this just has root or whatever build user

  C. build_rapiodev.sh

     REQUIREMENTS: mrmsbasedev (Since we're building the code)
     USAGE: Build rapio in container from source.  So it requires compilers, headers, etc.

  D. build_rapioops.sh

     REQUIREMENTS: mrmsbasedev, mrmsbasesops (Since we're building the code and deploying)
     USAGE: Small binary deploy of RAPIO files

  E. build_uncrustify.sh

     REQUIREMENTS: None
     USAGE: Build a uncrustify image, so that the check_format.sh and formatcode.sh
            scripts work.  This is a program to pretty format the RAPIO source tree one
            folder up, which I use to consistency keep code the same.

Notes for (C).  RAPIO development image

This image expects a host folder for putting configuration files (say colormaps) and for cross-compiling your algorithm.
The reason is so you 'keep' code, etc. safely on the host and easily changeable.

Set up input, output and algorithm development folders on the host:

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

Use ./run_container.sh to start up new container
   If successful, you should have a /INPUT /OUTPUT and /MYALGS folders on the container.

Doxygen/dot is on the container.  If you run the container and go to /BUILD/MYALGS
 you can run the makedocs.sh script to generate doxygen documentation on the host.
